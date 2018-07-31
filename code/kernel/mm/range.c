
#include <mm/range.h>
#include <mm/common.h>
#include <mm/pmm.h>
#include <mm/vmm.h>
#include <stdlib/assert.h>

bool range_alloc(uintptr_t addr_start, size_t len, vm_acc_t flags)
{
  assert((len % FRAME_SIZE) == 0);

  for (uintptr_t addr = addr_start, addr_end = addr + len; addr < addr_end;)
  {
    size_t remaining = addr_end - addr;

    /* try to use a 1G frame */
    if ((addr % FRAME_SIZE_1G) == 0 && remaining >= FRAME_SIZE_1G)
    {
      uintptr_t frame = pmm_allocs(SIZE_1G);
      if (frame)
      {
        if (vmm_maps(addr, frame, flags, SIZE_1G))
        {
          addr += FRAME_SIZE_1G;
          continue;
        }
        else
        {
          pmm_frees(frame, SIZE_1G);
        }
      }
    }

    /* try to use a 2M frame */
    if ((addr % FRAME_SIZE_2M) == 0 && remaining >= FRAME_SIZE_2M)
    {
      uintptr_t frame = pmm_allocs(SIZE_2M);
      if (frame)
      {
        if (vmm_maps(addr, frame, flags, SIZE_2M))
        {
          addr += FRAME_SIZE_2M;
          continue;
        }
        else
        {
          pmm_frees(frame, SIZE_2M);
        }
      }
    }

    /* try to use a 4K frame */
    uintptr_t frame = pmm_alloc();
    if (!frame)
    {
      range_free(addr_start, len);
      return false;
    }

    if (!vmm_map(addr, frame, flags))
    {
      pmm_free(frame);
      range_free(addr_start, len);
      return false;
    }

    addr += FRAME_SIZE;
  }

  return true;
}

void range_free(uintptr_t addr, size_t len)
{
  assert((len % FRAME_SIZE) == 0);

  for (uintptr_t addr_end = addr + len; addr < addr_end;)
  {
    int size = vmm_size(addr);
    if (size != -1)
      pmm_free(vmm_unmap(addr));

    switch (size)
    {
      case SIZE_1G:
        addr += FRAME_SIZE_1G;
        break;

      case SIZE_2M:
        addr += FRAME_SIZE_2M;
        break;
      
      default:
        addr += FRAME_SIZE;
        break;
    }
  }
}
