#ifndef SHIFTLAB_CONSTANTS_H__
#define SHIFTLAB_CONSTANTS_H__

#define PMEM_MMAP_HINT (0x10000000000UL)
#define PMEM_MMAP_HINT_STR "PMEM_MMAP_HINT"

#define PRED_CONFIDENCE_BITS (2)
#define PRED_CONFIDENCE_MAX (1<<PRED_CONFIDENCE_BITS)

#define PRED_CONF_THRESHOLD (1)
#define PRED_CONF_INVALID (-1)

#define ENABLE_DW "ENABLE_DW"
#define ENABLE_EV "ENABLE_EV"

const size_t IHB_SIZE = 5;

#endif // SHIFTLAB_CONSTANTS_H__