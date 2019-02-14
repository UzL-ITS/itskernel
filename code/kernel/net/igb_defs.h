#pragma once

/*
Intel network controller register and flag definitions.
Taken from Linux e1000e driver, and slightly modified.
*/

/* Registers */
typedef enum
{
	IGB_REG_CTRL = 0x00000, /* Device Control - RW */
	IGB_REG_STATUS = 0x00008, /* Device Status - RO */
	IGB_REG_EECD = 0x00010, /* EEPROM/Flash Control - RW */
	IGB_REG_EERD = 0x00014, /* EEPROM Read - RW */
	IGB_REG_CTRL_EXT = 0x00018, /* Extended Device Control - RW */
	IGB_REG_FLA = 0x0001C, /* Flash Access - RW */
	IGB_REG_MDIC = 0x00020, /* MDI Control - RW */
	IGB_REG_SCTL = 0x00024, /* SerDes Control - RW */
	IGB_REG_FCAL = 0x00028, /* Flow Control Address Low - RW */
	IGB_REG_FCAH = 0x0002C, /* Flow Control Address High -RW */
	IGB_REG_FEXT = 0x0002C, /* Future Extended - RW */
	IGB_REG_FEXTNVM = 0x00028, /* Future Extended NVM - RW */
	IGB_REG_FEXTNVM3 = 0x0003C, /* Future Extended NVM 3 - RW */
	IGB_REG_FEXTNVM4 = 0x00024, /* Future Extended NVM 4 - RW */
	IGB_REG_FEXTNVM6 = 0x00010, /* Future Extended NVM 6 - RW */
	IGB_REG_FEXTNVM7 = 0x000E4, /* Future Extended NVM 7 - RW */
	IGB_REG_FEXTNVM9 = 0x5BB4, /* Future Extended NVM 9 - RW */
	IGB_REG_FEXTNVM11 = 0x5BBC, /* Future Extended NVM 11 - RW */
	IGB_REG_PCIEANACFG = 0x00F18, /* PCIE Analog Config */
	IGB_REG_FCT = 0x00030, /* Flow Control Type - RW */
	IGB_REG_VET = 0x00038, /* VLAN Ether Type - RW */
	IGB_REG_ICR = 0x01500, /* Interrupt Cause Read - R/clr */
	IGB_REG_ITR = 0x000C4, /* Interrupt Throttling Rate - RW */
	IGB_REG_ICS = 0x01504, /* Interrupt Cause Set - WO */
	IGB_REG_IMS = 0x01508, /* Interrupt Mask Set - RW */
	IGB_REG_IMC = 0x0150C, /* Interrupt Mask Clear - WO */
	IGB_REG_IAM = 0x01510, /* Interrupt Acknowledge Auto Mask */
	IGB_REG_IVAR = 0x01700, /* Interrupt Vector Allocation Register - RW */
	IGB_REG_SVCR = 0x000F0,
	IGB_REG_SVT = 0x000F4,
	IGB_REG_LPIC = 0x000FC, /* Low Power IDLE control */
	IGB_REG_RCTL = 0x00100, /* Rx Control - RW */
	IGB_REG_FCTTV = 0x00170, /* Flow Control Transmit Timer Value - RW */
	IGB_REG_TXCW = 0x00178, /* Tx Configuration Word - RW */
	IGB_REG_RXCW = 0x00180, /* Rx Configuration Word - RO */
	IGB_REG_PBA_ECC = 0x01100, /* PBA ECC Register */
	IGB_REG_TCTL = 0x00400, /* Tx Control - RW */
	IGB_REG_TCTL_EXT = 0x00404, /* Extended Tx Control - RW */
	IGB_REG_TIPG = 0x00410, /* Tx Inter-packet gap -RW */
	IGB_REG_AIT = 0x00458, /* Adaptive Interframe Spacing Throttle - RW */
	IGB_REG_LEDCTL = 0x00E00, /* LED Control - RW */
	IGB_REG_LEDMUX = 0x08130, /* LED MUX Control */
	IGB_REG_EXTCNF_CTRL = 0x00F00, /* Extended Configuration Control */
	IGB_REG_EXTCNF_SIZE = 0x00F08, /* Extended Configuration Size */
	IGB_REG_PHY_CTRL = 0x00F10, /* PHY Control Register in CSR */
	IGB_REG_PBA = 0x01000, /* Packet Buffer Allocation - RW */
	IGB_REG_PBS = 0x01008, /* Packet Buffer Size */
	IGB_REG_PBECCSTS = 0x0100C, /* Packet Buffer ECC Status - RW */
	IGB_REG_IOSFPC = 0x00F28, /* TX corrupted data  */
	IGB_REG_EEMNGCTL = 0x01010, /* MNG EEprom Control */
	IGB_REG_EEWR = 0x0102C, /* EEPROM Write Register - RW */
	IGB_REG_FLOP = 0x0103C, /* FLASH Opcode Register */
	IGB_REG_ERT = 0x02008, /* Early Rx Threshold - RW */
	IGB_REG_FCRTL = 0x02160, /* Flow Control Receive Threshold Low - RW */
	IGB_REG_FCRTH = 0x02168, /* Flow Control Receive Threshold High - RW */
	IGB_REG_PSRCTL = 0x02170, /* Packet Split Receive Control - RW */
	IGB_REG_RDFH = 0x02410, /* Rx Data FIFO Head - RW */
	IGB_REG_RDFT = 0x02418, /* Rx Data FIFO Tail - RW */
	IGB_REG_RDFHS = 0x02420, /* Rx Data FIFO Head Saved - RW */
	IGB_REG_RDFTS = 0x02428, /* Rx Data FIFO Tail Saved - RW */
	IGB_REG_RDFPC = 0x02430, /* Rx Data FIFO Packet Count - RW */
	IGB_REG_RDTR = 0x02820, /* Rx Delay Timer - RW */
	IGB_REG_RADV = 0x0282C, /* Rx Interrupt Absolute Delay Timer - RW */
	
	IGB_REG_RXCSUM = 0x05000, // Rx Checksum Control
	IGB_REG_RFCTL = 0x05008, // Receive Filter Control
	IGB_REG_MRQC = 0x05818, // Multiple Receive Control
	IGB_REG_RAL = 0x05400, // Receive Address Low
	IGB_REG_RAH = 0x05404, // Receive Address High
	IGB_REG_RDBAL = 0x02800, // RX Descriptor Base Address Low
	IGB_REG_RDBAH = 0x02804, // RX Descriptor Base Address High
	IGB_REG_RDLEN = 0x02808, // RX Descriptor Length
	IGB_REG_RDH = 0x02810, // RX Descriptor Head
	IGB_REG_RDT = 0x02818, // RX Descriptor Tail
	IGB_REG_TDBAL = 0x03800, // TX Descriptor Base Address Low
	IGB_REG_TDBAH = 0x03804, // TX Descriptor Base Address High
	IGB_REG_TDLEN = 0x03808, // TX Descriptor Length
	IGB_REG_TDH = 0x03810, // TX Descriptor Head
	IGB_REG_TDT = 0x03818, // TX Descriptor Tail
	IGB_REG_TXDCTL0 = 0xE028, // Transmit Descriptor Control #0
	IGB_REG_TXDCTL1 = IGB_REG_TXDCTL0 + 1 * 0x40, // Transmit Descriptor Control #1
	IGB_REG_TXDCTL2 = IGB_REG_TXDCTL0 + 2 * 0x40, // Transmit Descriptor Control #2
	IGB_REG_TXDCTL3 = IGB_REG_TXDCTL0 + 3 * 0x40, // Transmit Descriptor Control #3
	IGB_REG_RXDCTL0 = 0xC028, // Receive Descriptor Control #0
	IGB_REG_RXDCTL1 = IGB_REG_RXDCTL0 + 1 * 0x40, // Receive Descriptor Control #1
	IGB_REG_RXDCTL2 = IGB_REG_RXDCTL0 + 2 * 0x40, // Receive Descriptor Control #2
	IGB_REG_RXDCTL3 = IGB_REG_RXDCTL0 + 3 * 0x40, // Receive Descriptor Control #3
	
	IGB_REG_MTA = 0x05200
} e1000_register_t;

/* Device Control */
typedef enum
{
	IGB_CTRL_FD = 0x00000001, /* Full duplex.0=half; 1=full */
	IGB_CTRL_GIO_MASTER_DISABLE = 0x00000004, /* Blocks new Master reqs */
	IGB_CTRL_LRST = 0x00000008, /* Link reset. 0=normal,1=reset */
	IGB_CTRL_ASDE = 0x00000020, /* Auto-speed detect enable */
	IGB_CTRL_SLU = 0x00000040, /* Set link up (Force Link) */
	IGB_CTRL_ILOS = 0x00000080, /* Invert Loss-Of Signal */
	IGB_CTRL_SPD_SEL = 0x00000300, /* Speed Select Mask */
	IGB_CTRL_SPD_10 = 0x00000000, /* Force 10Mb */
	IGB_CTRL_SPD_100 = 0x00000100, /* Force 100Mb */
	IGB_CTRL_SPD_1000 = 0x00000200, /* Force 1Gb */
	IGB_CTRL_FRCSPD = 0x00000800, /* Force Speed */
	IGB_CTRL_FRCDPX = 0x00001000, /* Force Duplex */
	IGB_CTRL_LANPHYPC_OVERRIDE = 0x00010000, /* SW control of LANPHYPC */
	IGB_CTRL_LANPHYPC_VALUE = 0x00020000, /* SW value of LANPHYPC */
	IGB_CTRL_MEHE = 0x00080000, /* Memory Error Handling Enable */
	IGB_CTRL_SWDPIN0 = 0x00040000, /* SWDPIN 0 value */
	IGB_CTRL_SWDPIN1 = 0x00080000, /* SWDPIN 1 value */
	IGB_CTRL_ADVD3WUC = 0x00100000, /* D3 WUC */
	IGB_CTRL_EN_PHY_PWR_MGMT = 0x00200000, /* PHY PM enable */
	IGB_CTRL_SWDPIO0 = 0x00400000, /* SWDPIN 0 Input or output */
	IGB_CTRL_RST = 0x04000000, /* Global reset */
	IGB_CTRL_RFCE = 0x08000000, /* Receive Flow Control enable */
	IGB_CTRL_TFCE = 0x10000000, /* Transmit flow control enable */
	IGB_CTRL_VME = 0x40000000, /* IEEE VLAN mode enable */
	IGB_CTRL_PHY_RST = 0x80000000, /* PHY Reset */
} e1000_ctrl_flags_t;

/* Extended Device Control */
typedef enum
{
	IGB_CTRL_EXT_LPCD = 0x00000004, /* LCD Power Cycle Done */
	IGB_CTRL_EXT_SDP3_DATA = 0x00000080, /* SW Definable Pin 3 data */
	IGB_CTRL_EXT_FORCE_SMBUS = 0x00000800, /* Force SMBus mode */
	IGB_CTRL_EXT_EE_RST = 0x00002000, /* Reinitialize from EEPROM */
	IGB_CTRL_EXT_SPD_BYPS = 0x00008000, /* Speed Select Bypass */
	IGB_CTRL_EXT_RO_DIS = 0x00020000, /* Relaxed Ordering disable */
	IGB_CTRL_EXT_DMA_DYN_CLK_EN = 0x00080000, /* DMA Dynamic Clk Gating */
	IGB_CTRL_EXT_LINK_MODE_MASK = 0x00C00000,
	IGB_CTRL_EXT_LINK_MODE_PCIE_SERDES = 0x00C00000,
	IGB_CTRL_EXT_EIAME = 0x01000000,
	IGB_CTRL_EXT_DRV_LOAD = 0x10000000, /* Drv loaded bit for FW */
	IGB_CTRL_EXT_IAME = 0x08000000, /* Int ACK Auto-mask */
	IGB_CTRL_EXT_PBA_CLR = 0x80000000, /* PBA Clear */
	IGB_CTRL_EXT_LSECCK = 0x00001000,
	IGB_CTRL_EXT_PHYPDEN = 0x00100000,
} e1000_ctrl_ext_flags_t;

/* Device Status */
typedef enum
{
	IGB_STATUS_FD = 0x00000001, /* Duplex 0=half 1=full */
	IGB_STATUS_LU = 0x00000002, /* Link up.0=no,1=link */
	IGB_STATUS_TXOFF = 0x00000010, /* transmission paused */
	IGB_STATUS_SPEED_MASK = 0x000000C0,
	IGB_STATUS_SPEED_10 = 0x00000000, /* Speed 10Mb/s */
	IGB_STATUS_SPEED_100 = 0x00000040, /* Speed 100Mb/s */
	IGB_STATUS_SPEED_1000 = 0x00000080, /* Speed 1000Mb/s */
	IGB_STATUS_PHYRA = 0x00000400, /* PHY Reset Asserted */
	IGB_STATUS_GIO_MASTER_ENABLE = 0x00080000, /* Master request status */
	IGB_STATUS_PF_RST_DONE = 0x00200000, /* Software reset done */
} e1000_device_status_flags_t;

/* Receive Descriptor bit definitions */
typedef enum
{
	IGB_RXD_STAT_DD = 0x01, /* Descriptor Done */
	IGB_RXD_STAT_EOP = 0x02, /* End of Packet */
	IGB_RXD_STAT_IXSM = 0x04, /* Ignore checksum */
	IGB_RXD_STAT_VP = 0x08, /* IEEE VLAN Packet */
	IGB_RXD_STAT_UDPCS = 0x10, /* UDP xsum calculated */
	IGB_RXD_STAT_TCPCS = 0x20, /* TCP xsum calculated */
	IGB_RXD_ERR_CE = 0x01, /* CRC Error */
	IGB_RXD_ERR_SE = 0x02, /* Symbol Error */
	IGB_RXD_ERR_SEQ = 0x04, /* Sequence Error */
	IGB_RXD_ERR_CXE = 0x10, /* Carrier Extension Error */
	IGB_RXD_ERR_TCPE = 0x20, /* TCP/UDP Checksum Error */
	IGB_RXD_ERR_IPE = 0x40, /* IP Checksum Error */
	IGB_RXD_ERR_RXE = 0x80, /* Rx Data Error */
	IGB_RXD_SPC_VLAN_MASK = 0x0FFF, /* VLAN ID is in lower 12 bits */
} e1000_rx_decs_flags_t;

/* Receive Control */
typedef enum
{
	IGB_RCTL_EN = 0x00000002, /* enable */
	IGB_RCTL_SBP = 0x00000004, /* store bad packet */
	IGB_RCTL_UPE = 0x00000008, /* unicast promisc enable */
	IGB_RCTL_MPE = 0x00000010, /* multicast promisc enable */
	IGB_RCTL_LPE = 0x00000020, /* long packet enable */
	IGB_RCTL_LBM_NO = 0x00000000, /* no loopback mode */
	IGB_RCTL_LBM_MAC = 0x00000040, /* MAC loopback mode */
	IGB_RCTL_LBM_TCVR = 0x000000C0, /* tcvr loopback mode */
	IGB_RCTL_MO_SHIFT = 12, /* multicast offset shift */
	IGB_RCTL_MO_3 = 0x00003000, /* multicast offset 15:4 */
	IGB_RCTL_BAM = 0x00008000, /* broadcast enable */
	/* these buffer sizes are valid if IGB_RCTL_BSEX is 0 */
	IGB_RCTL_SZ_2048 = 0x00000000, /* Rx buffer size 2048 */
	IGB_RCTL_SZ_1024 = 0x00010000, /* Rx buffer size 1024 */
	IGB_RCTL_SZ_512 = 0x00020000, /* Rx buffer size 512 */
	IGB_RCTL_SZ_256 = 0x00030000, /* Rx buffer size 256 */
	IGB_RCTL_VFE = 0x00040000, /* vlan filter enable */
	IGB_RCTL_CFIEN = 0x00080000, /* canonical form enable */
	IGB_RCTL_CFI = 0x00100000, /* canonical form indicator */
	IGB_RCTL_DPF = 0x00400000, /* discard pause frames */
	IGB_RCTL_PMCF = 0x00800000, /* pass MAC control frames */
	IGB_RCTL_SECRC = 0x04000000, /* Strip Ethernet CRC */
} e1000_rx_ctrl_flags_t;

/* Transmit Descriptor bit definitions */
typedef enum
{
	IGB_TXD_DTYP_D = 0x00100000, /* Data Descriptor */
	IGB_TXD_POPTS_IXSM = 0x01, /* Insert IP checksum */
	IGB_TXD_POPTS_TXSM = 0x02, /* Insert TCP/UDP checksum */
	IGB_TXD_CMD_EOP = 0x01000000, /* End of Packet */
	IGB_TXD_CMD_IFCS = 0x02000000, /* Insert FCS (Ethernet CRC) */
	IGB_TXD_CMD_IC = 0x04000000, /* Insert Checksum */
	IGB_TXD_CMD_RS = 0x08000000, /* Report Status */
	IGB_TXD_CMD_RPS = 0x10000000, /* Report Packet Sent */
	IGB_TXD_CMD_DEXT = 0x20000000, /* Desc extension (0 = legacy) */
	IGB_TXD_CMD_VLE = 0x40000000, /* Add VLAN tag */
	IGB_TXD_CMD_IDE = 0x80000000, /* Enable Tidv register */
	IGB_TXD_STAT_DD = 0x00000001, /* Descriptor Done */
	IGB_TXD_STAT_EC = 0x00000002, /* Excess Collisions */
	IGB_TXD_STAT_LC = 0x00000004, /* Late Collisions */
	IGB_TXD_STAT_TU = 0x00000008, /* Transmit underrun */
	IGB_TXD_CMD_TCP = 0x01000000, /* TCP packet */
	IGB_TXD_CMD_IP = 0x02000000, /* IP packet */
	IGB_TXD_CMD_TSE = 0x04000000, /* TCP Seg enable */
	IGB_TXD_STAT_TC = 0x00000004, /* Tx Underrun */
	IGB_TXD_EXTCMD_TSTAMP = 0x00000010, /* IEEE1588 Timestamp packet */
} e1000_tx_desc_flags_t;

/* Transmit Control */
typedef enum
{
	IGB_TCTL_EN = 0x00000002, /* enable Tx */
	IGB_TCTL_PSP = 0x00000008, /* pad short packets */
	IGB_TCTL_CT = 0x00000ff0, /* collision threshold */
	IGB_TCTL_RTLC = 0x01000000, /* Re-transmit on late collision */
} e1000_tx_ctrl_flags_t;

/* Interrupt Cause Read */
typedef enum
{
	IGB_ICR_TXDW = 0x00000001, /* Transmit desc written back */
	IGB_ICR_TXQE = 0x00000002, /* Transmit queue empty */
	IGB_ICR_LSC = 0x00000004, /* Link Status Change */
	IGB_ICR_RXSEQ = 0x00000008, /* Rx sequence error */
	IGB_ICR_RXDMT0 = 0x00000010, /* Rx desc min. threshold (0) */
	IGB_ICR_RXT0 = 0x00000080, /* Rx timer intr (ring 0) */
	IGB_ICR_ECCER = 0x00400000, /* Uncorrectable ECC Error */
	IGB_ICR_INT_ASSERTED = 0x80000000, /* If this bit asserted, the driver should claim the interrupt */
	IGB_ICR_RXQ0 = 0x00100000, /* Rx Queue 0 Interrupt */
	IGB_ICR_RXQ1 = 0x00200000, /* Rx Queue 1 Interrupt */
	IGB_ICR_TXQ0 = 0x00400000, /* Tx Queue 0 Interrupt */
	IGB_ICR_TXQ1 = 0x00800000, /* Tx Queue 1 Interrupt */
	IGB_ICR_OTHER = 0x01000000, /* Other Interrupts */
} e1000_intr_cause_flags_t;

/* Interrupt Mask Set */
typedef enum
{
	IGB_IMS_TXDW = IGB_ICR_TXDW, /* Tx desc written back */
	IGB_IMS_LSC = IGB_ICR_LSC, /* Link Status Change */
	IGB_IMS_RXSEQ = IGB_ICR_RXSEQ, /* Rx sequence error */
	IGB_IMS_RXDMT0 = IGB_ICR_RXDMT0, /* Rx desc min. threshold */
	IGB_IMS_RXT0 = IGB_ICR_RXT0, /* Rx timer intr */
	IGB_IMS_ECCER = IGB_ICR_ECCER, /* Uncorrectable ECC Error */
	IGB_IMS_RXQ0 = IGB_ICR_RXQ0, /* Rx Queue 0 Interrupt */
	IGB_IMS_RXQ1 = IGB_ICR_RXQ1, /* Rx Queue 1 Interrupt */
	IGB_IMS_TXQ0 = IGB_ICR_TXQ0, /* Tx Queue 0 Interrupt */
	IGB_IMS_TXQ1 = IGB_ICR_TXQ1, /* Tx Queue 1 Interrupt */
	IGB_IMS_OTHER = IGB_ICR_OTHER, /* Other Interrupt */
} e1000_intr_mask_flags_t;