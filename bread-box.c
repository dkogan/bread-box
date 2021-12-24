#include <stdio.h>
#include <stdlib.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/time.h>
#include <inttypes.h>

#define I2C_BUS                  "/dev/i2c-1"
#define DEVICE_ADDR              0x40
#define CMD_MEASURE_HUMIDITY     0xF5
#define CMD_MEASURE_TEMPERATURE  0xF3
#define CMD_RETRIEVE_TEMPERATURE 0xE0
#define CMD_FIRMWARE             0x84,0xB8


// Realtime plot with
//
// feedgnuplot                   \
//   --domain                    \
//   --vnl                       \
//   --autolegend                \
//   --stream                    \
//   --lines                     \
//   --points                    \
//   --timefmt '%s'              \
//   --set 'format x "%H:%M:%S"' \
//   --y2 humidity


static
int64_t gettimeofday_int64(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int64_t)tv.tv_sec;
}

#define TRY(what, error) \
    if( !(what) ) { printf("## " error "\n"); return 1; }

int main(void)
{
    int fd;

    printf("# time temperature humidity\n");

    TRY((fd = open(I2C_BUS, O_RDWR)) >= 0,
        "Failed to open i2c bus");
    TRY(0 == ioctl(fd, I2C_SLAVE, DEVICE_ADDR),
        "I2C config failed");

    uint8_t data[2];

    TRY(2 == write(fd, (char[]){CMD_FIRMWARE}, 2),
        "Wrote Nbytes != 2");
    sleep(1);
    TRY(1 == read(fd, data, 1),
        "Read Nbytes != 1");
    printf("## Firmware byte: 0x%02x\n", data[0]);


    while(1)
    {
        TRY(1 == write(fd, (char[]){CMD_MEASURE_HUMIDITY}, 1),
            "Wrote Nbytes != 1");
        sleep(1);
        TRY(2 == read(fd, data, 2),
            "Read Nbytes != 2");
        double humidity =
            ((double)(data[0])*256. + (double)(data[1])) * 125./65536. - 6.;

        TRY(1 == write(fd, (char[]){CMD_RETRIEVE_TEMPERATURE}, 1),
            "Wrote Nbytes != 1");
        TRY(2 == read(fd, data, 2),
            "Read Nbytes != 2");
        double temperature =
            ((double)(data[0])*256. + (double)(data[1])) * 175.72/65536. - 46.85;

        printf("%" PRId64 " %.1f %.1f\n",
               gettimeofday_int64(), temperature, humidity);
        fflush(stdout);
    }

    return 0;
}
