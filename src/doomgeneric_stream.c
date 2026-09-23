// SPDX-License-Identifier: GPL-2.0-or-later
// Lossless indexed frames and key up/down events over a non-PTY SSH channel.
#include "doomgeneric.h"
#include "i_video.h"
#include "m_argv.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int DG_StreamActive;
static unsigned char palette[768];

void DG_SetStreamColor(unsigned index, unsigned r, unsigned g, unsigned b)
{
    palette[index * 3] = r;
    palette[index * 3 + 1] = g;
    palette[index * 3 + 2] = b;
}

#ifdef __linux__
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
static int stream_fd;

static void Send(const void *data, size_t length)
{
    const unsigned char *p = data;
    while (length) {
        ssize_t n = write(stream_fd, p, length);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) exit(1);
        p += n; length -= n;
    }
}

int DG_InitStream(void)
{
    if (!M_CheckParm("-pixel-stream")) return 0;
    fflush(stdout);
    stream_fd = dup(STDOUT_FILENO);
    if (stream_fd < 0 || dup2(STDERR_FILENO, STDOUT_FILENO) < 0) exit(1);
    if (fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK) < 0) exit(1);
    DG_StreamActive = 1;
    Send("LFDOOM1\n", 8);
    return 1;
}

void DG_DrawStream(void)
{
    static uint32_t sequence;
    unsigned char header[8];
    uint32_t ms = DG_GetTicksMs();
    for (unsigned i = 0; i < 4; ++i) {
        header[i] = sequence >> (8 * i);
        header[i + 4] = ms >> (8 * i);
    }
    ++sequence;
    Send(header, sizeof header);
    Send(palette, sizeof palette);
    Send(I_VideoBuffer, 320 * 200);
}

int DG_StreamKey(int *pressed, unsigned char *key)
{
    static unsigned char event[2];
    static unsigned used;
    while (used < sizeof event) {
        ssize_t n = read(STDIN_FILENO, event + used, sizeof event - used);
        if (n < 0 && errno == EINTR) continue;
        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) return 0;
        if (n <= 0) exit(n == 0 ? 0 : 1);
        used += n;
    }
    used = 0;
    *key = event[0]; *pressed = event[1] != 0;
    return 1;
}
#else
int DG_InitStream(void) { return 0; }
void DG_DrawStream(void) {}
int DG_StreamKey(int *pressed, unsigned char *key) { (void)pressed; (void)key; return 0; }
#endif
