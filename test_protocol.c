/* Synthetic USB: firmware probe must send QUERYINFO only, never RF writes. */
#define libusb_bulk_transfer mock_bulk_transfer
#define main sendts_main
#include "sendts-it9507.c"
#undef main
#include <assert.h>
#include <sys/wait.h>

static uint8_t sequence;
static uint16_t command;
static int calls, corrupt, stream_calls;
static int notify_fd = -1;

int mock_bulk_transfer(libusb_device_handle *handle, unsigned char endpoint,
                       unsigned char *data, int length, int *actual, unsigned int timeout)
{
    (void)handle; (void)timeout;
    calls++;
    if (endpoint == 0x06) {
        stream_calls++;
        if (notify_fd >= 0) assert(write(notify_fd, "x", 1) == 1);
        *actual = corrupt ? length - 188 : length;
        return 0;
    }
    if (endpoint == 0x02) {
        assert(length == 7);
        command = ((uint16_t)data[1] << 8) | data[2];
        assert(command == Command_QUERYINFO || command == (Processor_OFDM | Command_QUERYINFO));
        sequence = data[3];
        *actual = length;
        return 0;
    }
    assert(endpoint == 0x81 && length >= 9);
    uint32_t version = command == Command_QUERYINFO ? DVB_LINK_VERSION : DVB_OFDM_VERSION;
    data[0] = 8; data[1] = sequence; data[2] = 0;
    for (int i=0; i<4; i++) data[3+i] = version >> (24-8*i);
    uint16_t crc = it9507_checksum(data+1, 6);
    data[7] = crc >> 8; data[8] = crc;
    *actual = 9;
    if (corrupt == 1) data[0] = 1;
    if (corrupt == 2) data[1]++;
    if (corrupt == 3) data[8] ^= 1;
    if (corrupt == 4) *actual = 8;
    return 0;
}

int main(void)
{
    struct it950x_dev dev = {0};
    assert(it9507_get_fwver(&dev, Processor_LINK) == DVB_LINK_VERSION);
    assert(it9507_get_fwver(&dev, Processor_OFDM) == DVB_OFDM_VERSION);
    assert(calls == 4 && dev.error == 0);
    for (corrupt=1; corrupt<=4; corrupt++) {
        dev.error = 0;
        assert(it9507_get_fwver(&dev, Processor_LINK) == 0xffffffffU);
        assert(dev.error != 0);
    }
    corrupt = 0;
    FILE *ts = tmpfile(); assert(ts);
    unsigned char packet[188] = {0x47, 0x1f, 0xff, 0x10};
    fwrite(packet, 1, sizeof(packet), ts); rewind(ts);
    assert(it950x_stream_data(&dev, fileno(ts)) == 0);
    assert(stream_calls == 1);
    rewind(ts); corrupt = 1;
    assert(it950x_stream_data(&dev, fileno(ts)) == -EIO);
    assert(stream_calls == 2); /* A short USB write is not retried/duplicated. */
    rewind(ts); corrupt = 0; stopping = 1;
    assert(it950x_stream_data(&dev, fileno(ts)) == 0 && stream_calls == 2);
    fclose(ts);
    stopping = 0; ts = tmpfile(); assert(ts);
    fwrite(packet, 1, 187, ts); rewind(ts);
    assert(it950x_stream_data(&dev, fileno(ts)) == -EINVAL && stream_calls == 2);
    fclose(ts);
    /* Sparse VBR must reach USB before EOF/full batch, even when pipe reads
     * split one TS packet across two output batches. No NULLs are synthesized. */
    int source[2], notification[2];
    assert(pipe(source) == 0 && pipe(notification) == 0);
    pid_t child = fork(); assert(child >= 0);
    if (child == 0) {
        close(source[0]); close(notification[1]);
        struct pollfd ack = { .fd = notification[0], .events = POLLIN };
        unsigned char bytes[376], reply;
        memcpy(bytes, packet, 188); memcpy(bytes + 188, packet, 188);
        if (write(source[1], bytes, 225) != 225) _exit(10);
        if (poll(&ack, 1, 1000) != 1 || read(ack.fd, &reply, 1) != 1) _exit(11);
        if (write(source[1], bytes + 225, 151) != 151) _exit(12);
        if (poll(&ack, 1, 1000) != 1 || read(ack.fd, &reply, 1) != 1) _exit(13);
        close(source[1]); close(notification[0]); _exit(0);
    }
    close(source[1]); close(notification[0]);
    notify_fd = notification[1]; int previous = stream_calls;
    assert(it950x_stream_data(&dev, source[0]) == 0);
    close(source[0]); close(notification[1]); notify_fd = -1;
    int child_status; assert(waitpid(child, &child_status, 0) == child);
    assert(WIFEXITED(child_status) && WEXITSTATUS(child_status) == 0);
    assert(stream_calls == previous + 2);
    puts("USB probe/parser and TS transfer checks passed");
    return 0;
}
