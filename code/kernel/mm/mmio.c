
#include <mm/mmio.h>
#include <mm/align.h>
#include <mm/heap.h>
#include <mm/vmm.h>

void *mmio_map(uintptr_t phy, size_t len, vm_acc_t flags)
{
  /* align the address and pad with extra length */
  uintptr_t aligned_phy = PAGE_ALIGN_REVERSE(phy);
  size_t off = phy - aligned_phy;
  len += off;

  /* reserve some virtual memory in the heap */
  uintptr_t aligned_virt = (uintptr_t) heap_reserve(len);
  if (!aligned_virt)
    return 0;

  /* map the physical memory into virtual memory */
  if (!vmm_map_range(aligned_virt, aligned_phy, len, flags))
  {
    heap_free((void *) aligned_virt);
    return 0;
  }

  /* return where the memory is mapped */
  return (void *) (aligned_virt + off);

}

void mmio_unmap(void *virt, size_t len)
{
  /* align the address and pad with extra length */
  uintptr_t aligned_virt = PAGE_ALIGN_REVERSE((uintptr_t) virt);
  size_t off = (uintptr_t) virt - aligned_virt;
  len += off;

  /* unmap the physical memory */
  vmm_unmap_range(aligned_virt, len);

  /* unreserve the virtual memory in the heap */
  heap_free((void *) aligned_virt);
}
