
#include <mm/vmm.h>
#include <proc/proc.h>
#include <lock/spinlock.h>
#include <mm/tlb.h>
#include <mm/align.h>
#include <mm/pmm.h>
#include <mm/mmio.h>
#include <panic/panic.h>
#include <cpu/tlb.h>
#include <cpu/features.h>
#include <stddef.h>
#include <stdlib/string.h>
#include <trace/trace.h>

// Does 4 loops using PML4[510] -> Pointer to PML4 itself.
#define PML4_OFFSET 0xFFFFFF7FBFDFE000

// Does 3 loops using PML4[510] -> Pointer to virtual memory for 510 PML3 tables.
#define PML3_OFFSET 0xFFFFFF7FBFC00000

// Does 2 loops using PML4[510] -> Pointer to virtual memory for 510 * 512 PML2 tables.
#define PML2_OFFSET 0xFFFFFF7F80000000

// Does 1 loop using PML4[510] -> Pointer to virtual memory for 510 * 512 * 512 PML1 tables.
#define PML1_OFFSET 0xFFFFFF0000000000

typedef struct
{
  uint64_t *pml4, *pml3, *pml2, *pml1;
  size_t pml4index, pml3index, pml2index, pml1index;
} page_index_t;

static spinlock_t kernel_vmm_lock = SPIN_UNLOCKED;

/* forward declarations of internal vmm functions with no locking */
static bool _vmm_touch(uintptr_t virt, int size);
static bool _vmm_map(uintptr_t virt, uintptr_t phy, vm_acc_t flags);
static bool _vmm_maps(uintptr_t virt, uintptr_t phy, vm_acc_t flags, int size);
static uintptr_t _vmm_unmap(uintptr_t virt);
static uintptr_t _vmm_unmaps(uintptr_t virt, int size);
static void _vmm_untouch(uintptr_t virt, int size);
static bool _vmm_map_range(uintptr_t virt, uintptr_t phy, size_t len, vm_acc_t flags);
static void _vmm_unmap_range(uintptr_t virt, size_t len);
static int _vmm_size(uintptr_t virt);

static void vmm_lock(uintptr_t addr)
{
  proc_t *proc = proc_get();
  if(proc && addr < VM_HIGHER_HALF)
    spin_lock(&proc->vmm_lock);
  else
    spin_lock(&kernel_vmm_lock);
}

static void vmm_unlock(uintptr_t addr)
{
  proc_t *proc = proc_get();
  if(proc && addr < VM_HIGHER_HALF)
    spin_unlock(&proc->vmm_lock);
  else
    spin_unlock(&kernel_vmm_lock);
}

static void addr_to_index(page_index_t *index, uintptr_t addr)
{
	/* calculate pml4 pointer */
	index->pml4 = (uint64_t *)PML4_OFFSET;

	/* calculate pml4 index */
	// This is needed since the last 16 address bits are a copy of address bit 47 (sign extension)
	if(addr >= VM_HIGHER_HALF)
	{
		addr -= VM_HIGHER_HALF;
		index->pml4index = (PAGE_TABLE_ENTRY_COUNT / 2) + addr / FRAME_SIZE_512G;
	}
	else
	{
		index->pml4index = addr / FRAME_SIZE_512G;
	}
	addr %= FRAME_SIZE_512G;

	/* calculate pml3 pointer */
	index->pml3 = (uint64_t *)(PML3_OFFSET + index->pml4index * FRAME_SIZE);

	/* calculate pml3 index */
	index->pml3index = addr / FRAME_SIZE_1G;
	addr %= FRAME_SIZE_1G;

	/* calculate pml2 pointer */
	index->pml2 = (uint64_t *)(PML2_OFFSET + index->pml4index * FRAME_SIZE_2M + index->pml3index * FRAME_SIZE);

	/* calculate pml2 index */
	index->pml2index = addr / FRAME_SIZE_2M;
	addr %= FRAME_SIZE_2M;

	/* calculate pml1 pointer */
	index->pml1 = (uint64_t *)(PML1_OFFSET + index->pml4index * FRAME_SIZE_1G + index->pml3index * FRAME_SIZE_2M + index->pml2index * FRAME_SIZE);

	/* calculate pml1 index */
	index->pml1index = addr / FRAME_SIZE;
}

void vmm_init(void)
{
	/*
	* touch all higher half pml4 entries, this means when we have multiple
	* address spaces, we can easily keep the higher half mapped in exactly the
	* same way by not creating all higher half pml4 entries now and never
	* changing them again
	*/
	// The loop accesses each non-user-space entry of PML4; this leads to allocation of an empty PML3 table for each of these
	// When a new user-space process is created, we copy the entire higher half of the PML4; this way,
	// we can do kernel-space operations without switching the virtual address space during system calls (we don't care for security
	// against the Meltdown attack here).
	for(int pml4_index = (PAGE_TABLE_ENTRY_COUNT / 2); pml4_index <= PAGE_TABLE_ENTRY_COUNT; pml4_index++)
	{
		if(!_vmm_touch(VM_HIGHER_HALF + (pml4_index - (PAGE_TABLE_ENTRY_COUNT / 2)) * FRAME_SIZE_512G, SIZE_1G))
			panic("failed to touch pml4 entry %d", pml4_index);
	}
}

bool vmm_init_pml4(uintptr_t pml4_table_addr)
{
	// TODO: pre-allocate this MMIO area so this call always succeeds
	uint64_t *pml4_table = mmio_map(pml4_table_addr, FRAME_SIZE, VM_R | VM_W);
	if(!pml4_table)
		return false;

	uint64_t *master_pml4_table = (uint64_t *)PML4_OFFSET;

	/* reset the lower half PML4 entries */
	// The lower half is user space, the higher half kernel space
	memset(pml4_table, 0, FRAME_SIZE / 2);

	/* copy the higher half PML4 entries from the master table */
	// (see vmm_init)
	for(int pml4index = PAGE_TABLE_ENTRY_COUNT / 2; pml4index < PAGE_TABLE_ENTRY_COUNT; pml4index++)
		pml4_table[pml4index] = master_pml4_table[pml4index];

	/* map PML4 into itself */
	pml4_table[PAGE_TABLE_ENTRY_COUNT - 2] = pml4_table_addr | PG_PRESENT | PG_WRITABLE | PG_NO_EXEC;

	mmio_unmap(pml4_table, FRAME_SIZE);
	return true;
}

// Returns the page size for the given virtual address.
static int _vmm_size(uintptr_t virt)
{
	// Get page table indices
	page_index_t index;
	addr_to_index(&index, virt);

	// Check PML4 entry
	uint64_t pml4entry = index.pml4[index.pml4index];
	if(!(pml4entry & PG_PRESENT))
		return -1;

	// Check PML3 entry
	uint64_t pml3entry = index.pml3[index.pml3index];
	if(!(pml3entry & PG_PRESENT))
		return -1;
	if(pml3entry & PG_BIG)
		return SIZE_1G;

	// Check PML2 entry
	uint64_t pml2entry = index.pml2[index.pml2index];
	if(!(pml2entry & PG_PRESENT))
		return -1;
	if(pml2entry & PG_BIG)
		return SIZE_2M;

	// Check PML1 entry
	uint64_t pml1entry = index.pml1[index.pml1index];
	if(!(pml1entry & PG_PRESENT))
		return -1;
	return SIZE_4K;
}

// Makes sure that the page table structure for the given address with the given size exists.
static bool _vmm_touch(uintptr_t virt, int size)
{
	// Get page table indices for the given address
	page_index_t index;
	addr_to_index(&index, virt);  

	// Check whether this address resides in user space
	bool isUserSpacePage = (index.pml4index < (PAGE_TABLE_ENTRY_COUNT / 2));

	// Add new PML3 entry to PML4, if it does not exist yet
	uint64_t pml4entry = index.pml4[index.pml4index];
	uintptr_t frame3 = 0;
	if(!(pml4entry & PG_PRESENT))
	{
		// Create PML3 table
		frame3 = pmm_alloc();
		if(!frame3)
			return false;

		// Set entry flags
		pml4entry = frame3 | PG_WRITABLE | PG_PRESENT;
		if(isUserSpacePage)
			pml4entry |= PG_USER;

		// Add entry to PML4
		index.pml4[index.pml4index] = pml4entry;
		tlb_transaction_queue_invlpg((uintptr_t)index.pml3);
		
		// Make sure new PML3 is empty
		memset(index.pml3, 0, FRAME_SIZE);
	}

	// 1G pages only need a PML3 entry
	if(size == SIZE_1G)
		return true;
	
	// Add new PML2 entry to PML3, if it does not exist yet
	uint64_t pml3entry = index.pml3[index.pml3index]; // This should be 0 when PML3 was just allocated
	uintptr_t frame2 = 0;
	if(pml3entry & PG_BIG) // TODO why??
		goto rollback_pml4;
	if(!(pml3entry & PG_PRESENT))
	{
		// Create PML2 table
		frame2 = pmm_alloc();
		if(!frame2)
			goto rollback_pml4;

		// Set entry flags
		pml3entry = frame2 | PG_WRITABLE | PG_PRESENT;
		if(isUserSpacePage)
			pml3entry |= PG_USER;

		// Add entry to PML3
		index.pml3[index.pml3index] = pml3entry;
		tlb_transaction_queue_invlpg((uintptr_t)index.pml2);
		
		// Make sure new PML2 is empty
		memset(index.pml2, 0, FRAME_SIZE);
	}

	// 2M pages only need a PML2 entry
	if(size == SIZE_2M)
		return true;

	// Add new PML1 entry to PML2, if it does not exist yet
	uint64_t pml2entry = index.pml2[index.pml2index];
	if(pml2entry & PG_BIG) // TODO why??
		goto rollback_pml3;
	if(!(pml2entry & PG_PRESENT))
	{
		// Create PML1 table
		uintptr_t frame1 = pmm_alloc();
		if(!frame1)
			goto rollback_pml3;
		
		// Set entry flags
		pml2entry = frame1 | PG_WRITABLE | PG_PRESENT;
		if(isUserSpacePage)
			pml2entry |= PG_USER;

		// Add entry to PML2
		index.pml2[index.pml2index] = pml2entry;
		tlb_transaction_queue_invlpg((uintptr_t)index.pml1);
		
		// Make sure new PML1 is empty
		memset(index.pml1, 0, FRAME_SIZE);
	}
	
	// All PMLs created
	return true;
	
	// Deletes the newly created PML3 entry again.
rollback_pml3:
	if(frame2)
	{
		index.pml3[index.pml3index] = 0;
		tlb_transaction_queue_invlpg((uintptr_t)index.pml2);
		pmm_free(frame2);
	}
	
	// Deletes the newly created PML4 entry again.
rollback_pml4:
	if(frame3)
	{
		index.pml4[index.pml4index] = 0;
		tlb_transaction_queue_invlpg((uintptr_t)index.pml3);
		pmm_free(frame3);
	}
	return false;
}

static bool _vmm_map(uintptr_t virt, uintptr_t phy, vm_acc_t flags)
{
  return _vmm_maps(virt, phy, flags, SIZE_4K);
}

// Assigns the given physical address to the given virtual address by modifying the respective page table entry.
static bool _vmm_maps(uintptr_t virt, uintptr_t phy, vm_acc_t flags, int size)
{
	// 1G pages supported?
	if(size == SIZE_1G && !enable1gPages)
		return false;

	// Build page table structure
	if(!_vmm_touch(virt, size))
		return false;

	// Retrieve page table indices
	page_index_t index;
	addr_to_index(&index, virt);

	// Derive page table flags
	uint64_t pg_flags = 0;
	if(flags & VM_W)
		pg_flags |= PG_WRITABLE;
	if(!(flags & VM_X))
		pg_flags |= PG_NO_EXEC;
	if(index.pml4index < (PAGE_TABLE_ENTRY_COUNT / 2))
		pg_flags |= PG_USER;

	// Write page table entry depending on size
	switch(size)
	{
		case SIZE_4K:
			index.pml1[index.pml1index] = phy | PG_PRESENT | pg_flags;
			break;

		case SIZE_2M:
			index.pml2[index.pml2index] = phy | PG_PRESENT | PG_BIG | pg_flags;
			break;

		case SIZE_1G:
			index.pml3[index.pml3index] = phy | PG_PRESENT | PG_BIG | pg_flags;
			break;
	}

	// Reset TLB for given virtual address
	tlb_transaction_queue_invlpg(virt);
	return true;
}

static uintptr_t _vmm_unmap(uintptr_t virt)
{
  return _vmm_unmaps(virt, SIZE_4K);
}

static uintptr_t _vmm_unmaps(uintptr_t virt, int size)
{
  page_index_t index;
  addr_to_index(&index, virt);

  uintptr_t frame = 0;
  switch(size)
  {
    case SIZE_4K:
      if(index.pml1[index.pml1index] & PG_PRESENT)
        frame = index.pml1[index.pml1index] & PG_ADDR_MASK;
      index.pml1[index.pml1index] = 0;
      break;

    case SIZE_2M:
      if(index.pml2[index.pml2index] & PG_PRESENT)
        frame = index.pml2[index.pml2index] & PG_ADDR_MASK;
      index.pml2[index.pml2index] = 0;
      break;

    case SIZE_1G:
      if(index.pml3[index.pml3index] & PG_PRESENT)
        frame = index.pml3[index.pml3index] & PG_ADDR_MASK;
      index.pml3[index.pml3index] = 0;
      break;
  }

  tlb_transaction_queue_invlpg(virt);
  _vmm_untouch(virt, size);
  return frame;
}

static void _vmm_untouch(uintptr_t virt, int size)
{
  page_index_t index;
  addr_to_index(&index, virt);

  if(size == SIZE_4K)
  {
    bool empty = true;
    for(size_t i = 0; i < PAGE_TABLE_ENTRY_COUNT; i++)
    {
      if(index.pml1[i] & PG_PRESENT)
      {
        empty = false;
        break;
      }
    }

    if(empty)
    {
      pmm_free(index.pml2[index.pml2index] & PG_ADDR_MASK);
      index.pml2[index.pml2index] = 0;
      tlb_transaction_queue_invlpg((uintptr_t) index.pml1);
    }
  }

  if(size == SIZE_4K || size == SIZE_2M)
  {
    bool empty = true;
    for(size_t i = 0; i < PAGE_TABLE_ENTRY_COUNT; i++)
    {
      if(index.pml2[i] & PG_PRESENT)
      {
        empty = false;
        break;
      }
    }

    if(empty)
    {
      pmm_free(index.pml3[index.pml3index] & PG_ADDR_MASK);
      index.pml3[index.pml3index] = 0;
      tlb_transaction_queue_invlpg((uintptr_t) index.pml2);
    }
  }

  if((size == SIZE_4K || size == SIZE_2M || size == SIZE_1G) && (index.pml4index < (PAGE_TABLE_ENTRY_COUNT / 2)))
  {
    bool empty = true;
    for(size_t i = 0; i < PAGE_TABLE_ENTRY_COUNT; i++)
    {
      if(index.pml3[i] & PG_PRESENT)
      {
        empty = false;
        break;
      }
    }

    if(empty)
    {
      pmm_free(index.pml4[index.pml4index] & PG_ADDR_MASK);
      index.pml4[index.pml4index] = 0;
      tlb_transaction_queue_invlpg((uintptr_t) index.pml3);
    }
  }
}

static bool _vmm_map_range(uintptr_t virt, uintptr_t phy, size_t len, vm_acc_t flags)
{
  len = PAGE_ALIGN(len);
  for(size_t off = 0; off < len;)
  {
    size_t remaining = len - off;
    if((PAGE_ALIGN_1G(virt + off) == (virt + off)) && remaining >= FRAME_SIZE_1G)
    {
      if(_vmm_maps(virt + off, phy + off, flags, SIZE_1G))
      {
        off += FRAME_SIZE_1G;
        continue;
      }
    }

    if((PAGE_ALIGN_2M(virt + off) == (virt + off)) && remaining >= FRAME_SIZE_2M)
    {
      if(_vmm_maps(virt + off, phy + off, flags, SIZE_2M))
      {
        off += FRAME_SIZE_2M;
        continue;
      }
    }

    if(!_vmm_map(virt + off, phy + off, flags))
    {
      _vmm_unmap_range(virt, off);
      return false;
    }

    off += FRAME_SIZE;
  }
  return true;
}

static void _vmm_unmap_range(uintptr_t virt, size_t len)
{
  len = PAGE_ALIGN(len);
  for(size_t off = 0; off < len;)
  {
    int size = _vmm_size(virt + off);
    if(size != -1)
      _vmm_unmaps(size, virt + off);

    if(size == SIZE_1G)
      off += FRAME_SIZE_1G;
    else if(size == SIZE_2M)
      off += FRAME_SIZE_2M;
    else
      off += FRAME_SIZE;
  }
}

bool vmm_touch(uintptr_t virt, int size)
{
  vmm_lock(virt);

  tlb_transaction_init();
  bool ok = _vmm_touch(virt, size);
  if(ok)
    tlb_transaction_commit();
  else
    tlb_transaction_rollback();

  vmm_unlock(virt);
  return ok;
}

bool vmm_map(uintptr_t virt, uintptr_t phy, vm_acc_t flags)
{
  vmm_lock(virt);

  tlb_transaction_init();
  bool ok = _vmm_map(virt, phy, flags);
  if(ok)
    tlb_transaction_commit();
  else
    tlb_transaction_rollback();

  vmm_unlock(virt);
  return ok;
}

bool vmm_maps(uintptr_t virt, uintptr_t phy, vm_acc_t flags, int size)
{
  vmm_lock(virt);

  tlb_transaction_init();
  bool ok = _vmm_maps(virt, phy, flags, size);
  if(ok)
    tlb_transaction_commit();
  else
    tlb_transaction_rollback();

  vmm_unlock(virt);
  return ok;
}

uintptr_t vmm_unmap(uintptr_t virt)
{
  vmm_lock(virt);
  tlb_transaction_init();
  uintptr_t addr = _vmm_unmap(virt);
  tlb_transaction_commit();
  vmm_unlock(virt);
  return addr;
}

uintptr_t vmm_unmaps(uintptr_t virt, int size)
{
  vmm_lock(virt);
  tlb_transaction_init();
  uintptr_t addr = _vmm_unmaps(virt, size);
  tlb_transaction_commit();
  vmm_unlock(virt);
  return addr;
}

void vmm_untouch(uintptr_t virt, int size)
{
  vmm_lock(virt);
  tlb_transaction_init();
  _vmm_untouch(virt, size);
  tlb_transaction_commit();
  vmm_unlock(virt);
}

bool vmm_map_range(uintptr_t virt, uintptr_t phy, size_t len, vm_acc_t flags)
{
  vmm_lock(virt);
  tlb_transaction_init();
  bool ok = _vmm_map_range(virt, phy, len, flags);
  if(ok)
    tlb_transaction_commit();
  else
    tlb_transaction_rollback();

  vmm_unlock(virt);
  return ok;
}

void vmm_unmap_range(uintptr_t virt, size_t len)
{
  vmm_lock(virt);
  tlb_transaction_init();
  _vmm_unmap_range(virt, len);
  tlb_transaction_commit();
  vmm_unlock(virt);
}

int vmm_size(uintptr_t virt)
{
  vmm_lock(virt);
  int size = _vmm_size(virt);
  vmm_unlock(virt);
  return size;
}
