#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>      // for exit() and itoa() 
#include <fcntl.h>       // for open()
#include <sys/ioctl.h>   // for ioctl()
#include <unistd.h>      // for read()  and write()

#include <getopt.h>  
#include <string.h>
#include <errno.h>      // to fetch IOCTL errors

//#include <i2c.h>   // for i2c_msg - THis should define I2C_M_RD also ??

#include <signal.h>

/* cn9130 Design:

   BUS: CP0_I2C0_Sxx

      MPP37, mpp 38

      MCP23017T  Microchip 16 port I/O Expander.
      address:  0x27 (7bit address) on the Schematics calls out

      LM73C1QDSDCRQ1  Temp Sensor
      Address  = 0x4c (7bit address) on the Schematics calls out

      24LC256-I/SN   EEPROM
      Address: 0x57  (7bit address) on the Schematics calls out

      DDR4 DIMM   
      Address: 0x53  

   BUS: CP0_I2C1_Sxx

      MPP35, mpp 36

      SFP+_20P_CONN
      Address: 0xA0, 0xA2  8 BIt?     
*/

  #include <linux/i2c-dev.h>
  #include <i2c/smbus.h>


int g_i2c_file = 0;
int g_i2c_verbose = 0;
#define I2C_VERBOSE(x)    if(g_i2c_verbose > x )


int g_qflg = 0;      /* flag to suppress everyting but the read 
                      * rsults.  Note, writes will return nothing
                      * if successful  else error;
                      */ 


int g_probe = 0;      /* probe function - will eventuall print all devices on the bus */

#define NOT_QUIET (g_qflg ==0)

int  I2C_Init(char * device);
void I2C_Close(void);
int  I2C_RdWr(int slave_addr,uint8_t * wr_ptr,int64_t wr_sz,uint8_t * rd_ptr,int64_t rd_sz);

int  GetHex(unsigned char d);
void PrintBuff(char * ptr , int sz);

// Exception handling
/* Signal Handler for SIGINT */
void sigintHandler(int sig_num) 
{ 
    /* Reset handler to catch SIGINT next time. 
       Refer http://en.cppreference.com/w/c/program/signal */
    signal(SIGINT, sigintHandler); 
    printf("\n Terminate \n");
    fflush(stdout); 
    I2C_Close();
    exit(-77); 
} 




void print_usage(void)
{
   printf("twsi_tool \n");
   printf("-d <DeviceBusName>   EG: /dev/i2c-0  (default)\n");
   printf("-a <address>         I2c Devices's Address\n");
   printf("-w 02,0a   write bytes - HEX, comma separated,NO SPACES \n");
   printf("-r <num>       read bytes - number of butes to reads  \n");
   printf("-p             probe - when it works will return addresses for all devices on bus\n");
   printf("-v <num>            Verbose Flag  \n");
   printf("example: \n");
   printf("./twsi_tool  -d /dev/i2c-1  -a 4c -w 07  -r 2  \n");
   printf("./twsi_tool  -q -d /dev/i2c-1  -a 4c -w 07  -r 2  \n");
//   printf("./twsi_tool  -m \n");
   printf("\n");
}



int main(int argc, char ** argv)
{

  int file;
  char filename[20];
  int       index;
  int       result = 0 ;

  char      devicename[256];  
  uint16_t  i2c_slave_addr = 0x4C; /* The I2C address */
  int       wr_sz=0;
  char      wr_buf[2048]; 
  int       wr_flg=0;
  char      writeString[2048] = {0}; 

  int       rd_sz=0;
  char      rd_buf[2048]; 

  int       option ;
  int       UsageFlag = 0;
  int       addr;

// set up /dev/i2c-0 as the default device bus
   strcpy(devicename, "/dev/i2c-0");


   while ((option = getopt(argc, argv,"hqpsv:l:a:d:w:r:")) != -1) {
       switch (option) {

            case 'a' : 
//              UsageFlag++ ;               // Flag we had a good input
                i2c_slave_addr = GetHex(*optarg);  // #of times to send File Data
                i2c_slave_addr *= 16;
                i2c_slave_addr += GetHex(*(optarg+1));
                break;

            case 'd' : 
                strcpy(devicename,optarg);  // Serial Port
                break;

            case 'p' : 
                UsageFlag++ ;               // Flag we had a good input
                g_probe = 1 ;               // flag to do Probe Function.
                break;


            case 'q' : 
                g_qflg = 1;                 // global quiet flag
                break;

            case 'r' : 
                UsageFlag++ ;               // Flag we had a good input
                rd_sz = atoi(optarg);       // #of times to send File Data
                break;

             case 'w' : 
                UsageFlag++ ;               // Flag we had a good input
                strcpy(writeString,optarg); // #of times to send File Data
                wr_flg = 1;
                break;
 
            case 'v' : 
                g_i2c_verbose = atoi(optarg);  // #of times to send File Data
                break;
            case 'h' :                               //  Help
                print_usage();
//                exit(EXIT_FAILURE);
                return(1);
       }
   }

   if( UsageFlag == 0)
   {
      print_usage();
      return 2;
   }


/* Set the SIGINT (Ctrl-C) signal handler to sigintHandler  
       Refer http://en.cppreference.com/w/c/program/signal */
    signal(SIGINT, sigintHandler); 


// work out the write data....
// Write data is expected to be a sequence of 2 hex digits separated by
//    a space.  the code will NOT BE BEHAVED if there is only a single
//    digit, or there is a 0x.
   if (wr_flg != 0)  
   {
     int    done = 0;    
     char  *ptr  = writeString;
     int    val  = 0;
           index = 0;     
     if( NOT_QUIET ) printf(" writeString: %s\n",writeString); 
     while (done == 0)
     {
         I2C_VERBOSE(1)  printf(" *prt: %c  %02x\n",*ptr,*ptr); 
         if (*ptr == 0) 
         {
           wr_buf[index] = val;
           index++;
           done = 1;
         }     
         else if (*ptr == ',')
         {
           wr_buf[index] = val;
           val = 0;
           index++;
           ptr++;
         }
         else
         {
           val = val*16;
           val = val + GetHex(*ptr);
           ptr++;
         }
     } 
     wr_sz = index;
   }

  if ( NOT_QUIET )
  {
       printf("Command Line Arguments:\n");
       printf("   %12s Device Name\n",devicename);
       printf("%s 0x%02x Slave Address\n","          ",i2c_slave_addr);
       printf("   %12d Verbose\n",(int)g_i2c_verbose);
       printf("   %12d Read Size\n",(int)rd_sz);
       printf("   %12d Write Size\n",(int)wr_sz);
       printf("   %12d Probe Flag\n",(int)g_probe);

       if( wr_flg != 0)
       {
           printf("Write Data:");
           for ( index = 0 ; index  < wr_sz ; index++)
           {
#ifdef PRINT_WRITE_DATA_WITH_INDEX_COUNT
              if( index%16 == 0) printf("\n%3d",index);
#else
              if( index%16 == 0) printf("\n");
#endif
              if( index%8 == 0) printf("  ");
              printf("%02x ", wr_buf[index] );
           }
       }
       printf("\n");
   }  // end NOT_QUITE

// test rd_size_limits
     if ( rd_sz < 0 )
     {
        printf("ERROR: Read Size was < 0\n");
        return -1;
     }  
     if ( rd_sz > 255 )
     {
        printf("ERROR: Read Size was > 255\n");
        return -2;
     }  

#if 0
//  test function to dump the contents of the LM73 on the CRB
   DumpLM73();
#endif 

//  Attach to /dev/i2c-<n> 
   if( result = I2C_Init(devicename) != 0)
   {
      printf("Unable to open %s\n",devicename);
      printf("- exiting -\n");

      return result;
   }

   if (g_probe == 1)
   {
      uint8_t addr;
      wr_buf[0] = 0;
      wr_sz      = 1;
      rd_sz      = 0;
      rd_buf[0] = 0;
      
      for (addr = 1 ; addr < 0x7f; addr ++)
      {
         result = I2C_RdWr(addr ,wr_buf,wr_sz,rd_buf,rd_sz);
         if(result == 0)
         {
         printf("Addr: 0x%02x, Device Present\n",addr);
         }
         else
         {
         printf("Addr: 0x%02x, result=%d\n",addr,result);
         }
      }
   }  // end the probe function
   else
   {

// execute the Read Write function
      result = I2C_RdWr(i2c_slave_addr ,wr_buf,wr_sz,rd_buf,rd_sz);


      if ( NOT_QUIET ) printf("Read Data:");
      for ( index = 0 ; index  < rd_sz ; index++)
      {
           if ( NOT_QUIET )
           {
#     ifdef PRINT_WRITE_DATA_WITH_INDEX_COUNT
                if( index%16 == 0) printf("\n%3d",index);
#else
                if( index%16 == 0) printf("\n");
#endif
                if( index%8 == 0) printf("  ");
            } // end not quiet
            printf("%02x ", rd_buf[index] );
       }
       printf("\n");

//     PrintBuff(rd_buf,rd_sz);
    } // end a read or write operation


// close the devicename file
   I2C_Close();

}  // end main




#ifdef DEVELOPMENT_CODE

// the reference code for below can be found in NOTES_CN9130


  int       adapter_nr = 0; /* /dev/i2c-%d - probably dynamically 
                             * determined 
                             */
 
  snprintf(filename, 19, "/dev/i2c-%d", adapter_nr);
  file = open(filename, O_RDWR);
  if (file < 0) {
    /* ERROR HANDLING; you can check errno to see what went wrong */
    exit(1);
  }
  
/* When you have opened the device, you must specify with what device
 8 address you want to communicate:
*/
  int addr = i2c_slave_addr; /* The I2C address */

  if (ioctl(file, I2C_SLAVE, addr) < 0) {
    /* ERROR HANDLING; you can check errno to see what went wrong */
    exit(1);
  }


  char buf[256];


int test = 0;

if(test == 1 )
{
   printf("******************** TEST read()  *************\n");
   int read_actual=0;
  /* Using I2C Read, equivalent of i2c_smbus_read_byte(file) */
  if ((read_actual = read(file, buf, rd_sz)) != rd_sz) {
    /* ERROR HANDLING: i2c transaction failed */
    printf("%d-%s:%s  RE  - Unable to read %d byte\n",__LINE__,__FILE__,__FUNCTION__,rd_sz);
  } else {
   /* buf[0] contains the read byte */
     printf("RE BUS:/dev/i2c-%d Addr: 0x%-2x, Data: 0x%02x\n", adapter_nr,addr,buf[0]);
  }

   PrintBuff( buf , read_actual);

  printf("\n");
}

   printf("************* TEST ioctl(I2C_RDWR)  ********\n");
   struct i2c_rdwr_ioctl_data {
      struct i2c_msg *msgs;  /* ptr to array of simple messages */
      int nmsgs;             /* number of messages to exchange */
   };

#define BUFF_SIZE 32
   int result;

   uint8_t    i2c_wr_buf[BUFF_SIZE]; 
   uint16_t   i2c_wr_buf_sz;
   uint8_t    i2c_rd_buf[BUFF_SIZE];
   uint16_t   i2c_rd_buf_sz;

   struct i2c_msg rw_msg[2];
   struct i2c_rdwr_ioctl_data  i2c_rdwr_data;

   for ( index = 0 ; index < BUFF_SIZE ;index ++)
   {
      i2c_wr_buf[index]=0;
      i2c_rd_buf[index]=0;
   }


   i2c_wr_buf[0]=0x01;     // write reg select 1
   i2c_wr_buf_sz = 1;
   
   rw_msg[0].addr = i2c_slave_addr;
   rw_msg[0].flags = 0;
   rw_msg[0].len = i2c_wr_buf_sz;
   rw_msg[0].buf = i2c_wr_buf;

//   i2c_rd_buf_sz = 2;    // read 2 bytes?
   i2c_rd_buf_sz = rd_sz;    // read 2 bytes?

   rw_msg[1].addr = i2c_slave_addr;
   rw_msg[1].flags = I2C_M_RD;
   rw_msg[1].len = i2c_rd_buf_sz;
   rw_msg[1].buf = i2c_rd_buf;

   i2c_rdwr_data.msgs = &(rw_msg[0]);
   i2c_rdwr_data.nmsgs = 2 ;

//   ioctl(file, I2C_RDWR, struct i2c_rdwr_ioctl_data *msgset)
   result = ioctl(file, I2C_RDWR, &i2c_rdwr_data);
 
   printf("result: %d \n",result);  // the result looks like the number of Msgs transacted

   PrintBuff( i2c_rd_buf , i2c_rd_buf_sz);

   close(file);

#endif


void DumpLM73(void)
{
int result;
uint8_t  wr_buf[32];
int64_t  wr_sz;
uint8_t  rd_buf[32];
int64_t  rd_sz;

int slave_addr=0x4c;

result= I2C_Init("/dev/i2c-0");

printf(" LM73 registers @0x%02x:\n",slave_addr);

wr_buf[0]=0x00; 
wr_sz = 1;
rd_sz = 2;
result = I2C_RdWr(slave_addr,wr_buf,wr_sz,rd_buf,rd_sz);
printf(" 0x%02x temp          %02x %02x \n",wr_buf[0],rd_buf[0],rd_buf[1]);

wr_buf[0]=0x01; 
wr_sz = 1;
rd_sz = 1;
result = I2C_RdWr(slave_addr,wr_buf,wr_sz,rd_buf,rd_sz);
printf(" 0x%02x config        %02x    \n",wr_buf[0],rd_buf[0]);


wr_buf[0]=0x02; 
wr_sz = 1;
rd_sz = 2;
result = I2C_RdWr(slave_addr,wr_buf,wr_sz,rd_buf,rd_sz);
printf(" 0x%02x Upper Limit   %02x %02x \n",wr_buf[0],rd_buf[0],rd_buf[1]);

wr_buf[0]=0x03; 
wr_sz = 1;
rd_sz = 2;
result = I2C_RdWr(slave_addr,wr_buf,wr_sz,rd_buf,rd_sz);
printf(" 0x%02x Lower Limit   %02x %02x \n",wr_buf[0],rd_buf[0],rd_buf[1]);

wr_buf[0]=0x04; 
wr_sz = 1;
rd_sz = 1;
result = I2C_RdWr(slave_addr,wr_buf,wr_sz,rd_buf,rd_sz);
printf(" 0x%02x Ctrl/Status   %02x    \n",wr_buf[0],rd_buf[0]);

wr_buf[0]=0x07; 
wr_sz = 1;
rd_sz = 2;
result = I2C_RdWr(slave_addr,wr_buf,wr_sz,rd_buf,rd_sz);
printf(" 0x%02x ID Register   %02x %02x \n",wr_buf[0],rd_buf[0],rd_buf[1]);

printf("\n");

// PrintBuff(rd_buf,rd_sz);

I2C_Close();

return; 

}

/************************************************************
 ***********************************************************/

int I2C_Init(char * device_name)
{
  if(g_i2c_verbose > 0) printf("%d:%s-%s\n",__LINE__,__FILE__,__FUNCTION__);
 
  // device name should look like /dev/i2c-0 or /dev/i2c-1
  g_i2c_file = open(device_name, O_RDWR);

  if(g_i2c_verbose > 0) printf("  driver open:  g_i2c_file = %d\n",g_i2c_file);

  if (g_i2c_file < 0) 
  {
      /* ERROR HANDLING; you can check errno to see what went wrong */
      printf("   Problem opening device : %d\n",g_i2c_file);
      printf("          errno: %d   %s\n",errno,strerror(errno)); 
      return(1);
  }
  return 0;
} 

int I2C_RdWr(int slave_addr,uint8_t * wr_ptr,int64_t wr_sz,uint8_t * rd_ptr,int64_t rd_sz)
{
   int result;
   int expected_result = 0;

   struct i2c_msg rw_msg[2];
   struct i2c_rdwr_ioctl_data  i2c_rdwr_data;

   if(g_i2c_verbose > 0) printf("%d:%s-%s\n",__LINE__,__FILE__,__FUNCTION__);

// set the slave address
   if ( (result = ioctl( g_i2c_file, I2C_SLAVE, slave_addr)) < 0) 
   {
      /* ERROR HANDLING; you can check errno to see what went wrong */
      printf("   Problem setting Slave address: %d\n",result);
      printf("          errno: %d   %s\n",errno,strerror(errno));
      return(-1);
   }

// set up the images. 
   if( (wr_sz > 0) && (rd_sz > 0 ) )
   {
     rw_msg[0].addr  = slave_addr;
     rw_msg[0].flags = 0;
     rw_msg[0].len   = wr_sz;
     rw_msg[0].buf   = wr_ptr;

     rw_msg[1].addr  = slave_addr;
     rw_msg[1].flags = I2C_M_RD;
     rw_msg[1].len   = rd_sz;
     rw_msg[1].buf   = rd_ptr;

     i2c_rdwr_data.nmsgs = 2 ;
     expected_result = 2 ;

   }
   else if ( (wr_sz == 0) && (rd_sz > 0 ) )   /* read only */
   {
     rw_msg[0].addr  = slave_addr;
     rw_msg[0].flags = I2C_M_RD;
     rw_msg[0].len   = rd_sz;
     rw_msg[0].buf   = rd_ptr;

     i2c_rdwr_data.nmsgs = 1 ;
     expected_result = 1 ;
   }
    else if ( (wr_sz > 0) && (rd_sz == 0 ) )   /* write  only */
   {
     rw_msg[0].addr  = slave_addr;
     rw_msg[0].flags = 0;
     rw_msg[0].len   = wr_sz;
     rw_msg[0].buf   = wr_ptr;
  
     i2c_rdwr_data.nmsgs = 1 ;
     expected_result = 1 ;
   }
   else
   {
     printf("%d:%s-%s\n",__LINE__,__FILE__,__FUNCTION__);
     printf(" Parameter Errors:  wr_sz %ld  rd_sz  %ld \n",wr_sz,rd_sz);
     return -2;
   }
   
   i2c_rdwr_data.msgs = &(rw_msg[0]);

//   ioctl(file, I2C_RDWR, struct i2c_rdwr_ioctl_data *msgset)
   result = ioctl(g_i2c_file, I2C_RDWR, &i2c_rdwr_data);

   if( result != expected_result)
   {
     if(g_i2c_verbose > 0)printf("%d:%s-%s\n",__LINE__,__FILE__,__FUNCTION__);
     printf(" Result:%d did not match expected_result:%d \n",result,expected_result);
     printf("          errno: %d   %s\n",errno,strerror(errno));
   //  return -2;
   }
 
   //printf("result: %d \n",result);  // the result looks like the number of messages handled

   return result;
}

void I2C_Close(void)
{
   if(g_i2c_file != 0) 
   {
     close (g_i2c_file);
     if(g_i2c_verbose > 0) printf("%d:%s-%s\n",__LINE__,__FILE__,__FUNCTION__);
   }  
}




// dumps a character buffer to the screen
void PrintBuff(char * ptr , int sz)
{
  int index;

  for ( index = 0 ; index  < sz ; index++)
  {
    if( index%16 == 0) printf("\n%3d",index);
    if( index%8 == 0) printf("  ");
    printf("%02x ", *(ptr + index) );
  }
  printf("\n");
}


//return 0 for any non-hex character, else the value
int GetHex(unsigned char d)
{
   if (( d >= '0' ) && ( d <= '9' ))     return (0x0f&d);
   if ( (( d >= 'a' ) && ( d <= 'f' )) ||
        (( d >= 'A' ) && ( d <= 'F' )) ) return (9+(d & 0x0f)); 
    
   return 0;
}






