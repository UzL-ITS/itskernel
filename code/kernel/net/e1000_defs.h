/*
Intel network controller register and flag definitions.
Taken from Linux e1000e driver.
*/

/* Registers */
typedef enum
{
	E1000_REG_CTRL = 0x00000, /* Device Control - RW */
	E1000_REG_STATUS = 0x00008, /* Device Status - RO */
	E1000_REG_EECD = 0x00010, /* EEPROM/Flash Control - RW */
	E1000_REG_EERD = 0x00014, /* EEPROM Read - RW */
	E1000_REG_CTRL_EXT = 0x00018, /* Extended Device Control - RW */
	E1000_REG_FLA = 0x0001C, /* Flash Access - RW */
	E1000_REG_MDIC = 0x00020, /* MDI Control - RW */
	E1000_REG_SCTL = 0x00024, /* SerDes Control - RW */
	E1000_REG_FCAL = 0x00028, /* Flow Control Address Low - RW */
	E1000_REG_FCAH = 0x0002C, /* Flow Control Address High -RW */
	E1000_REG_FEXT = 0x0002C, /* Future Extended - RW */
	E1000_REG_FEXTNVM = 0x00028, /* Future Extended NVM - RW */
	E1000_REG_FEXTNVM3 = 0x0003C, /* Future Extended NVM 3 - RW */
	E1000_REG_FEXTNVM4 = 0x00024, /* Future Extended NVM 4 - RW */
	E1000_REG_FEXTNVM6 = 0x00010, /* Future Extended NVM 6 - RW */
	E1000_REG_FEXTNVM7 = 0x000E4, /* Future Extended NVM 7 - RW */
	E1000_REG_FEXTNVM9 = 0x5BB4, /* Future Extended NVM 9 - RW */
	E1000_REG_FEXTNVM11 = 0x5BBC, /* Future Extended NVM 11 - RW */
	E1000_REG_PCIEANACFG = 0x00F18, /* PCIE Analog Config */
	E1000_REG_FCT = 0x00030, /* Flow Control Type - RW */
	E1000_REG_VET = 0x00038, /* VLAN Ether Type - RW */
	E1000_REG_ICR = 0x000C0, /* Interrupt Cause Read - R/clr */
	E1000_REG_ITR = 0x000C4, /* Interrupt Throttling Rate - RW */
	E1000_REG_ICS = 0x000C8, /* Interrupt Cause Set - WO */
	E1000_REG_IMS = 0x000D0, /* Interrupt Mask Set - RW */
	E1000_REG_IMC = 0x000D8, /* Interrupt Mask Clear - WO */
	E1000_REG_IAM = 0x000E0, /* Interrupt Acknowledge Auto Mask */
	E1000_REG_IVAR = 0x000E4, /* Interrupt Vector Allocation Register - RW */
	E1000_REG_SVCR = 0x000F0,
	E1000_REG_SVT = 0x000F4,
	E1000_REG_LPIC = 0x000FC, /* Low Power IDLE control */
	E1000_REG_RCTL = 0x00100, /* Rx Control - RW */
	E1000_REG_FCTTV = 0x00170, /* Flow Control Transmit Timer Value - RW */
	E1000_REG_TXCW = 0x00178, /* Tx Configuration Word - RW */
	E1000_REG_RXCW = 0x00180, /* Rx Configuration Word - RO */
	E1000_REG_PBA_ECC = 0x01100, /* PBA ECC Register */
	E1000_REG_TCTL = 0x00400, /* Tx Control - RW */
	E1000_REG_TCTL_EXT = 0x00404, /* Extended Tx Control - RW */
	E1000_REG_TIPG = 0x00410, /* Tx Inter-packet gap -RW */
	E1000_REG_AIT = 0x00458, /* Adaptive Interframe Spacing Throttle - RW */
	E1000_REG_LEDCTL = 0x00E00, /* LED Control - RW */
	E1000_REG_LEDMUX = 0x08130, /* LED MUX Control */
	E1000_REG_EXTCNF_CTRL = 0x00F00, /* Extended Configuration Control */
	E1000_REG_EXTCNF_SIZE = 0x00F08, /* Extended Configuration Size */
	E1000_REG_PHY_CTRL = 0x00F10, /* PHY Control Register in CSR */
	E1000_REG_PBA = 0x01000, /* Packet Buffer Allocation - RW */
	E1000_REG_PBS = 0x01008, /* Packet Buffer Size */
	E1000_REG_PBECCSTS = 0x0100C, /* Packet Buffer ECC Status - RW */
	E1000_REG_IOSFPC = 0x00F28, /* TX corrupted data  */
	E1000_REG_EEMNGCTL = 0x01010, /* MNG EEprom Control */
	E1000_REG_EEWR = 0x0102C, /* EEPROM Write Register - RW */
	E1000_REG_FLOP = 0x0103C, /* FLASH Opcode Register */
	E1000_REG_ERT = 0x02008, /* Early Rx Threshold - RW */
	E1000_REG_FCRTL = 0x02160, /* Flow Control Receive Threshold Low - RW */
	E1000_REG_FCRTH = 0x02168, /* Flow Control Receive Threshold High - RW */
	E1000_REG_PSRCTL = 0x02170, /* Packet Split Receive Control - RW */
	E1000_REG_RDFH = 0x02410, /* Rx Data FIFO Head - RW */
	E1000_REG_RDFT = 0x02418, /* Rx Data FIFO Tail - RW */
	E1000_REG_RDFHS = 0x02420, /* Rx Data FIFO Head Saved - RW */
	E1000_REG_RDFTS = 0x02428, /* Rx Data FIFO Tail Saved - RW */
	E1000_REG_RDFPC = 0x02430, /* Rx Data FIFO Packet Count - RW */
	E1000_REG_RDTR = 0x02820, /* Rx Delay Timer - RW */
	E1000_REG_RADV = 0x0282C, /* Rx Interrupt Absolute Delay Timer - RW */
	
	E1000_REG_RXCSUM = 0x05000, // Rx Checksum Control
	E1000_REG_RFCTL = 0x05008, // Receive Filter Control
	E1000_REG_MRQC = 0x05818, // Multiple Receive Control
	E1000_REG_RAL = 0x05400, // Receive Address Low
	E1000_REG_RAH = 0x05404, // Receive Address High
	E1000_REG_RDBAL = 0x02800, // RX Descriptor Base Address Low
	E1000_REG_RDBAH = 0x02804, // RX Descriptor Base Address High
	E1000_REG_RDLEN = 0x02808, // RX Descriptor Length
	E1000_REG_RDH = 0x02810, // RX Descriptor Head
	E1000_REG_RDT = 0x02818, // RX Descriptor Tail
	E1000_REG_TDBAL = 0x03800, // TX Descriptor Base Address Low
	E1000_REG_TDBAH = 0x03804, // TX Descriptor Base Address High
	E1000_REG_TDLEN = 0x03808, // TX Descriptor Length
	E1000_REG_TDH = 0x03810, // TX Descriptor Head
	E1000_REG_TDT = 0x03818, // TX Descriptor Tail
	E1000_REG_TARC0 = 0x3840, // Transmit Arbitration Count
	E1000_REG_TARC1 = E1000_REG_TARC0 + 0x100, // Transmit Arbitration Count
	
	E1000_REG_MTA = 0x05200
} e1000_register_t;

/* Device Control */
typedef enum
{
	E1000_CTRL_FD = 0x00000001, /* Full duplex.0=half; 1=full */
	E1000_CTRL_GIO_MASTER_DISABLE = 0x00000004, /* Blocks new Master reqs */
	E1000_CTRL_LRST = 0x00000008, /* Link reset. 0=normal,1=reset */
	E1000_CTRL_ASDE = 0x00000020, /* Auto-speed detect enable */
	E1000_CTRL_SLU = 0x00000040, /* Set link up (Force Link) */
	E1000_CTRL_ILOS = 0x00000080, /* Invert Loss-Of Signal */
	E1000_CTRL_SPD_SEL = 0x00000300, /* Speed Select Mask */
	E1000_CTRL_SPD_10 = 0x00000000, /* Force 10Mb */
	E1000_CTRL_SPD_100 = 0x00000100, /* Force 100Mb */
	E1000_CTRL_SPD_1000 = 0x00000200, /* Force 1Gb */
	E1000_CTRL_FRCSPD = 0x00000800, /* Force Speed */
	E1000_CTRL_FRCDPX = 0x00001000, /* Force Duplex */
	E1000_CTRL_LANPHYPC_OVERRIDE = 0x00010000, /* SW control of LANPHYPC */
	E1000_CTRL_LANPHYPC_VALUE = 0x00020000, /* SW value of LANPHYPC */
	E1000_CTRL_MEHE = 0x00080000, /* Memory Error Handling Enable */
	E1000_CTRL_SWDPIN0 = 0x00040000, /* SWDPIN 0 value */
	E1000_CTRL_SWDPIN1 = 0x00080000, /* SWDPIN 1 value */
	E1000_CTRL_ADVD3WUC = 0x00100000, /* D3 WUC */
	E1000_CTRL_EN_PHY_PWR_MGMT = 0x00200000, /* PHY PM enable */
	E1000_CTRL_SWDPIO0 = 0x00400000, /* SWDPIN 0 Input or output */
	E1000_CTRL_RST = 0x04000000, /* Global reset */
	E1000_CTRL_RFCE = 0x08000000, /* Receive Flow Control enable */
	E1000_CTRL_TFCE = 0x10000000, /* Transmit flow control enable */
	E1000_CTRL_VME = 0x40000000, /* IEEE VLAN mode enable */
	E1000_CTRL_PHY_RST = 0x80000000, /* PHY Reset */
} e1000_ctrl_flags_t;

/* Extended Device Control */
typedef enum
{
	E1000_CTRL_EXT_LPCD = 0x00000004, /* LCD Power Cycle Done */
	E1000_CTRL_EXT_SDP3_DATA = 0x00000080, /* SW Definable Pin 3 data */
	E1000_CTRL_EXT_FORCE_SMBUS = 0x00000800, /* Force SMBus mode */
	E1000_CTRL_EXT_EE_RST = 0x00002000, /* Reinitialize from EEPROM */
	E1000_CTRL_EXT_SPD_BYPS = 0x00008000, /* Speed Select Bypass */
	E1000_CTRL_EXT_RO_DIS = 0x00020000, /* Relaxed Ordering disable */
	E1000_CTRL_EXT_DMA_DYN_CLK_EN = 0x00080000, /* DMA Dynamic Clk Gating */
	E1000_CTRL_EXT_LINK_MODE_MASK = 0x00C00000,
	E1000_CTRL_EXT_LINK_MODE_PCIE_SERDES = 0x00C00000,
	E1000_CTRL_EXT_EIAME = 0x01000000,
	E1000_CTRL_EXT_DRV_LOAD = 0x10000000, /* Drv loaded bit for FW */
	E1000_CTRL_EXT_IAME = 0x08000000, /* Int ACK Auto-mask */
	E1000_CTRL_EXT_PBA_CLR = 0x80000000, /* PBA Clear */
	E1000_CTRL_EXT_LSECCK = 0x00001000,
	E1000_CTRL_EXT_PHYPDEN = 0x00100000,
} e1000_ctrl_ext_flags_t;

/* Device Status */
typedef enum
{
	E1000_STATUS_FD = 0x00000001, /* Duplex 0=half 1=full */
	E1000_STATUS_LU = 0x00000002, /* Link up.0=no,1=link */
	E1000_STATUS_FUNC_MASK = 0x0000000C, /* PCI Function Mask */
	E1000_STATUS_FUNC_SHIFT = 2,
	E1000_STATUS_FUNC_1 = 0x00000004, /* Function 1 */
	E1000_STATUS_TXOFF = 0x00000010, /* transmission paused */
	E1000_STATUS_SPEED_MASK = 0x000000C0,
	E1000_STATUS_SPEED_10 = 0x00000000, /* Speed 10Mb/s */
	E1000_STATUS_SPEED_100 = 0x00000040, /* Speed 100Mb/s */
	E1000_STATUS_SPEED_1000 = 0x00000080, /* Speed 1000Mb/s */
	E1000_STATUS_LAN_INIT_DONE = 0x00000200, /* Lan Init Compltn by NVM */
	E1000_STATUS_PHYRA = 0x00000400, /* PHY Reset Asserted */
	E1000_STATUS_GIO_MASTER_ENABLE = 0x00080000, /* Master request status */
	E1000_STATUS_2P5_SKU = 0x00001000, /* Val of 2.5GBE SKU strap */
	E1000_STATUS_2P5_SKU_OVER = 0x00002000, /* Val of 2.5GBE SKU Over */
} e1000_device_status_flags_t;

/* Receive Descriptor bit definitions */
typedef enum
{
	E1000_RXD_STAT_DD = 0x01, /* Descriptor Done */
	E1000_RXD_STAT_EOP = 0x02, /* End of Packet */
	E1000_RXD_STAT_IXSM = 0x04, /* Ignore checksum */
	E1000_RXD_STAT_VP = 0x08, /* IEEE VLAN Packet */
	E1000_RXD_STAT_UDPCS = 0x10, /* UDP xsum calculated */
	E1000_RXD_STAT_TCPCS = 0x20, /* TCP xsum calculated */
	E1000_RXD_ERR_CE = 0x01, /* CRC Error */
	E1000_RXD_ERR_SE = 0x02, /* Symbol Error */
	E1000_RXD_ERR_SEQ = 0x04, /* Sequence Error */
	E1000_RXD_ERR_CXE = 0x10, /* Carrier Extension Error */
	E1000_RXD_ERR_TCPE = 0x20, /* TCP/UDP Checksum Error */
	E1000_RXD_ERR_IPE = 0x40, /* IP Checksum Error */
	E1000_RXD_ERR_RXE = 0x80, /* Rx Data Error */
	E1000_RXD_SPC_VLAN_MASK = 0x0FFF, /* VLAN ID is in lower 12 bits */
} e1000_rx_decs_flags_t;

/* Receive Control */
typedef enum
{
	E1000_RCTL_EN = 0x00000002, /* enable */
	E1000_RCTL_SBP = 0x00000004, /* store bad packet */
	E1000_RCTL_UPE = 0x00000008, /* unicast promisc enable */
	E1000_RCTL_MPE = 0x00000010, /* multicast promisc enable */
	E1000_RCTL_LPE = 0x00000020, /* long packet enable */
	E1000_RCTL_LBM_NO = 0x00000000, /* no loopback mode */
	E1000_RCTL_LBM_MAC = 0x00000040, /* MAC loopback mode */
	E1000_RCTL_LBM_TCVR = 0x000000C0, /* tcvr loopback mode */
	E1000_RCTL_DTYP_PS = 0x00000400, /* Packet Split descriptor */
	E1000_RCTL_RDMTS_HALF = 0x00000000, /* Rx desc min thresh size */
	E1000_RCTL_RDMTS_HEX = 0x00010000,
	E1000_RCTL_RDMTS1_HEX = E1000_RCTL_RDMTS_HEX,
	E1000_RCTL_MO_SHIFT = 12, /* multicast offset shift */
	E1000_RCTL_MO_3 = 0x00003000, /* multicast offset 15:4 */
	E1000_RCTL_BAM = 0x00008000, /* broadcast enable */
	/* these buffer sizes are valid if E1000_RCTL_BSEX is 0 */
	E1000_RCTL_SZ_2048 = 0x00000000, /* Rx buffer size 2048 */
	E1000_RCTL_SZ_1024 = 0x00010000, /* Rx buffer size 1024 */
	E1000_RCTL_SZ_512 = 0x00020000, /* Rx buffer size 512 */
	E1000_RCTL_SZ_256 = 0x00030000, /* Rx buffer size 256 */
	/* these buffer sizes are valid if E1000_RCTL_BSEX is 1 */
	E1000_RCTL_SZ_16384 = 0x00010000, /* Rx buffer size 16384 */
	E1000_RCTL_SZ_8192 = 0x00020000, /* Rx buffer size 8192 */
	E1000_RCTL_SZ_4096 = 0x00030000, /* Rx buffer size 4096 */
	E1000_RCTL_VFE = 0x00040000, /* vlan filter enable */
	E1000_RCTL_CFIEN = 0x00080000, /* canonical form enable */
	E1000_RCTL_CFI = 0x00100000, /* canonical form indicator */
	E1000_RCTL_DPF = 0x00400000, /* discard pause frames */
	E1000_RCTL_PMCF = 0x00800000, /* pass MAC control frames */
	E1000_RCTL_BSEX = 0x02000000, /* Buffer size extension */
	E1000_RCTL_SECRC = 0x04000000, /* Strip Ethernet CRC */
} e1000_rx_ctrl_flags_t;

/* Transmit Descriptor bit definitions */
typedef enum
{
	E1000_TXD_DTYP_D = 0x00100000, /* Data Descriptor */
	E1000_TXD_POPTS_IXSM = 0x01, /* Insert IP checksum */
	E1000_TXD_POPTS_TXSM = 0x02, /* Insert TCP/UDP checksum */
	E1000_TXD_CMD_EOP = 0x01000000, /* End of Packet */
	E1000_TXD_CMD_IFCS = 0x02000000, /* Insert FCS (Ethernet CRC) */
	E1000_TXD_CMD_IC = 0x04000000, /* Insert Checksum */
	E1000_TXD_CMD_RS = 0x08000000, /* Report Status */
	E1000_TXD_CMD_RPS = 0x10000000, /* Report Packet Sent */
	E1000_TXD_CMD_DEXT = 0x20000000, /* Desc extension (0 = legacy) */
	E1000_TXD_CMD_VLE = 0x40000000, /* Add VLAN tag */
	E1000_TXD_CMD_IDE = 0x80000000, /* Enable Tidv register */
	E1000_TXD_STAT_DD = 0x00000001, /* Descriptor Done */
	E1000_TXD_STAT_EC = 0x00000002, /* Excess Collisions */
	E1000_TXD_STAT_LC = 0x00000004, /* Late Collisions */
	E1000_TXD_STAT_TU = 0x00000008, /* Transmit underrun */
	E1000_TXD_CMD_TCP = 0x01000000, /* TCP packet */
	E1000_TXD_CMD_IP = 0x02000000, /* IP packet */
	E1000_TXD_CMD_TSE = 0x04000000, /* TCP Seg enable */
	E1000_TXD_STAT_TC = 0x00000004, /* Tx Underrun */
	E1000_TXD_EXTCMD_TSTAMP = 0x00000010, /* IEEE1588 Timestamp packet */
} e1000_tx_desc_flags_t;

/* Transmit Control */
typedef enum
{
	E1000_TCTL_EN = 0x00000002, /* enable Tx */
	E1000_TCTL_PSP = 0x00000008, /* pad short packets */
	E1000_TCTL_CT = 0x00000ff0, /* collision threshold */
	E1000_TCTL_COLD = 0x003ff000, /* collision distance */
	E1000_TCTL_RTLC = 0x01000000, /* Re-transmit on late collision */
	E1000_TCTL_MULR = 0x10000000, /* Multiple request support */
} e1000_tx_ctrl_flags_t;

/* Interrupt Cause Read */
typedef enum
{
	E1000_ICR_TXDW = 0x00000001, /* Transmit desc written back */
	E1000_ICR_LSC = 0x00000004, /* Link Status Change */
	E1000_ICR_RXSEQ = 0x00000008, /* Rx sequence error */
	E1000_ICR_RXDMT0 = 0x00000010, /* Rx desc min. threshold (0) */
	E1000_ICR_RXT0 = 0x00000080, /* Rx timer intr (ring 0) */
	E1000_ICR_ECCER = 0x00400000, /* Uncorrectable ECC Error */
	E1000_ICR_INT_ASSERTED = 0x80000000, /* If this bit asserted, the driver should claim the interrupt */
	E1000_ICR_RXQ0 = 0x00100000, /* Rx Queue 0 Interrupt */
	E1000_ICR_RXQ1 = 0x00200000, /* Rx Queue 1 Interrupt */
	E1000_ICR_TXQ0 = 0x00400000, /* Tx Queue 0 Interrupt */
	E1000_ICR_TXQ1 = 0x00800000, /* Tx Queue 1 Interrupt */
	E1000_ICR_OTHER = 0x01000000, /* Other Interrupts */
} e1000_intr_cause_flags_t;

/* Interrupt Mask Set */
typedef enum
{
	E1000_IMS_TXDW = E1000_ICR_TXDW, /* Tx desc written back */
	E1000_IMS_LSC = E1000_ICR_LSC, /* Link Status Change */
	E1000_IMS_RXSEQ = E1000_ICR_RXSEQ, /* Rx sequence error */
	E1000_IMS_RXDMT0 = E1000_ICR_RXDMT0, /* Rx desc min. threshold */
	E1000_IMS_RXT0 = E1000_ICR_RXT0, /* Rx timer intr */
	E1000_IMS_ECCER = E1000_ICR_ECCER, /* Uncorrectable ECC Error */
	E1000_IMS_RXQ0 = E1000_ICR_RXQ0, /* Rx Queue 0 Interrupt */
	E1000_IMS_RXQ1 = E1000_ICR_RXQ1, /* Rx Queue 1 Interrupt */
	E1000_IMS_TXQ0 = E1000_ICR_TXQ0, /* Tx Queue 0 Interrupt */
	E1000_IMS_TXQ1 = E1000_ICR_TXQ1, /* Tx Queue 1 Interrupt */
	E1000_IMS_OTHER = E1000_ICR_OTHER, /* Other Interrupt */
} e1000_intr_mask_flags_t;