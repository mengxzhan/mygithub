//******************************************************************************
//* File name  : udu2_define.h
//* Date       : 04/08/04
//* Author     : Kevin Lo
//* Description: C defines for the UDU2 Module
//******************************************************************************
//------10--------20--------30--------40--------50--------60--------70--------80
#define  UDU2
#define  UDU2_IP_REVISION_ADDR      0x80000300
#define  UDU2_IP_REVISION_SIZE      16
#define  UDU2_IP_REVISION           0x0000

#define  USB_PHY 
#define  USB_PHY_IP_REVISION_ADDR   0x80000302
#define  USB_PHY_IP_REVISION_SIZE   16
#define  USB_PHY_IP_REVISION        0x0003

#define	 HIU_REG_BASE				0x80000000

#define	USB_BASE		0xD4208000
#define	UTMI_BASE		0xD4207000

#define	HSIC1_USB_BASE	0xF0001000		// HSIC1 USB CTRL 0xF0001000
#define	HSIC1_PHY_BASE	0xF0001800		// HSIC1 PHY      0xf0001800

#define	 C0_8618
#define  D1_8618

//       Register Name              Address       //Number            
#define  CARD_USB_CMD               (USB_BASE+0x140)         //         
#define  CARD_USB_CTL               (UTMI_BASE+0x04)         //4'h1         
#define  CARD_PHY_PLL_CTL           (UTMI_BASE+0x08)         //4'h2         
#define  CARD_PHY_TX_CTL            (UTMI_BASE+0x0C)         //4'h3         
#define  CARD_PHY_RX_CTL            (UTMI_BASE+0x10)         //4'h4         
#define  CARD_PHY_IVREF_CTL         (UTMI_BASE+0x14)         //4'h5         
#define  CARD_PHY_REG_TEST0         (UTMI_BASE+0x18)         //4'h6         
#define  CARD_PHY_REG_TEST1         (UTMI_BASE+0x1C)         //4'h7         
#define  CARD_PHY_REG_TEST2         (UTMI_BASE+0x20)         //4'h8         
#define  CARD_PHY_RESERVE           (UTMI_BASE+0x24)         //4'h9         
#define  CARD_USB_INT               (UTMI_BASE+0x28)         //4'hA         
#define  CARD_DBG_CTL               (UTMI_BASE+0x2C)         //4'hB         

//       Register Default           Value
#define  INIT_USB_CMD               0x00080000 
#define  INIT_USB_CTL               0x0 

/*
U2PLL    @0x5550a000 = 0xabc26eeb
U2PTX    @0x5550a004 = 0x41610fc3
U2PRX    @0x5550a008 = 0xe31c8271
U2PIVREF @0x5550a00c = 0x0000007e
U2PT0    @0x5550a010 = 0x10c01800
U2PT1    @0x5550a014 = 0xde000000
U2PT2    @0x5550a018 = 0x00000000

U2PT3    @0x5550a01c = 0x800f6000
*/
 


#define  INIT_PHY_PLL_CTL           0xabc26eeb	//0x2A01Aeeb	//768
#define  INIT_PHY_TX_CTL            0x41610fc3	//0x6107c3	//0x137C3			//0x117C1 		//0x107C3      //1 0000 1111 1100 0011
#define  INIT_PHY_RX_CTL            0xe31c8271	//0x6d1c82a3 //0x6D1C82A1		//0x7D5D0185   //111 1101 0101 1101 0000 0010 1000 0101
#define  INIT_PHY_IVREF_CTL         0x0000007e	//0x43a			//0x635        //110 0011 0101
#define  INIT_PHY_REG_TEST0         0x10c01800	//0xC09A21     //1100 0000 1001 1010 0000 0000 
#define  INIT_PHY_REG_TEST1         0xde000000	//0x00000000   //0000 0000 0000 0000 0000 0000 0000 0000 
#define  INIT_PHY_REG_TEST2         0x00000000	//0x0 
#define  INIT_PHY_RESERVE           0x0 
#define  INIT_USB_INT               0x0  
#define  INIT_DBG_CTL               0x9  


// Bit descriptions
//#define CARD_USB_CMD              32'h 
  #define REG_BIT_RST               1             
  #define REG_BIT_RS                0             

//#define CARD_USB_CTL              32'h 
  #define UTMI_CTRL_INPKT_DELAY_SOF_SHIFT 28
  #define REG_UTMI_CERR_CNT         27
  #define	REG_PU_REF				(20)	
  #define REG_UTMI_SELECT           8           
  #define REG_OPMODE                6  //7:6           
  #define REG_TERM_SELECT           5            
  #define REG_XCVR_SELECT           4            
  #define UDU2_TEST_MODE            2  //3:2        
  #define REG_BIT_PU_PLL            1             
  #define REG_BIT_PU                0             
  #define REG_RCAL_START                          0x00001000

//#define CARD_USB_PLL_CTL              32'h 
  #define UTMI_PLL_CLK_BLK_EN_SHIFT               24
  #define UTMI_PLL_FBDIV_SHIFT                    4
  #define UTMI_PLL_REFDIV_SHIFT                   0
  #define UTMI_PLL_FBDIV_MASK                     0x00000FF0
  #define UTMI_PLL_REFDIV_MASK                    0x0000000F
  #define VCOCAL_START                            0x00200000
  #define PLL_READY                               0x00800000

//#define CARD_PHY_REG_TEST1        32'h 
  #define REG_TEST_HS_HDL_SYNC      31
  #define REG_TEST_FS_HDL_SYNC      30
  #define REG_TEST_DIG_LPBK         20           
  #define REG_TEST_SKIP             17 //19:17           
  #define REG_TEST_FLAG             16           
  #define REG_TEST_DONE             15           
  #define REG_TEST_LENGTH           13 //14:13           
  #define REG_TEST_TX_PATTERN        5 //12:5           
  #define REG_TEST_MODE              3 //4:3           
  #define REG_TEST_LPBK_EN           2           
  #define REG_TEST_BYPASS            1           
  #define REG_TEST_EN                0           

//#define CARD_USB_INT              32'h 
  #define REG_LINE_INT              3           
  #define REG_LINE_INT_EN           1           
  #define REG_CORE_INT              0           


int usb_lpbk_test(int reg_test_length, int reg_test_mode, int reg_dig_lpbk );
int hsic_lpbk_test(int hsic_no, int length, int test_mode, int lpbk_path );
