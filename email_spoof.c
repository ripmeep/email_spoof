/*    email_spoof.c    */

/*
 * Author: ripmeep
 * GitHub: https://github.com/ripmeep/
 * Date  : 02/02/2023
 */

/* A simple email spoofer using sendmail, written
 * with the curl library using C/C++. */

/*    INCLUDES    */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>
#include <curl/curl.h>

/*    MACROS & STATIC DEFINITIONS    */
#define MAX_ADDRESS_LENGTH  1024
#define DEBUG               1

static const char* DAY_IDX[] = {
    "Sun", "Mon", "Tue",
    "Wed", "Thu", "Fri",
    "Sat"
};

static const char* MONTH_IDX[] = {
    "Jan", "Feb", "Mar", "Apr",
    "May", "Jun", "Jul", "Aug",
    "Sep", "Oct", "Nov", "Dec"
};

static const char INPUT_END[] = "END";

/*    TYPE DEFINITIONS    */
struct __upload_ctx
{
    char**  payload;
    int     lines;
};

typedef struct __email_t
{
    char                    from[MAX_ADDRESS_LENGTH];
    char                    to[MAX_ADDRESS_LENGTH];
    char                    subject[128];
    char                    time[32];
    char*                   body;

    struct curl_slist*      csl_recips;
    struct __upload_ctx*    us_ctx;

    size_t                  sz_recips;
} email_t;

static size_t __email_payload_read(void* ptr, size_t size,
                                   size_t nmemb, void* userp) __wur
{
    struct __upload_ctx*    us_ctx;
    char*                   data;
    size_t                  sz;

    if ((size == 0) || (nmemb == 0) || (size * nmemb) < 1)
        return 0;

    us_ctx = (struct __upload_ctx*)((email_t*)userp)->us_ctx;

    data = us_ctx->payload[us_ctx->lines];

    if (data)
    {
        sz = strlen(data);

        memcpy(ptr, data, sz);

        us_ctx->lines++;

        return sz;
    }

    return 0;
}

ssize_t strdate(char* __restrict__ md, size_t sz)
{
    ssize_t     ret;
    time_t      t;
    struct tm*  lcltm;

    time(&t);

    lcltm = localtime(&t);

    memset(md, 0, sz);

    ret = snprintf(md,
                   sz,
                   "%s, %d %s %d %02d:%02d:%02d",
                   DAY_IDX[lcltm->tm_wday],
                   lcltm->tm_mday,
                   MONTH_IDX[lcltm->tm_mon],
                   lcltm->tm_year + 1900,
                   lcltm->tm_hour,
                   lcltm->tm_min,
                   lcltm->tm_sec);

    return ret;
}

size_t input(const char* __restrict__ prompt,
             char*       __restrict__ md,
             size_t sz) __wur
{
    size_t  sz_len;

    memset(md, 0, sz);

    printf("%s", prompt);

    fgets(md, sz, stdin);

    sz_len = strlen(md);

    md[--sz_len] = '\0';

    return sz_len;
}

size_t input_lines(const char* __restrict__ prompt,
                   const char* __restrict__ end,
                   char** md) __wur
{
    char    line[512];
    size_t  sz_len, sz_line;

    *md = (char*)malloc(1);

    sz_len  = 0;
    sz_line = 0;

    assert(*md != NULL);

    while (1)
    {
        memset( line, 0, sizeof(line) );

        input( prompt, line, sizeof(line) - 1 );

        if (!strcmp(line, end))
            break;

        sz_line = strlen(line) + 1;

        line[sz_line - 1] = '\n';

        *md = (char*)realloc(*md, sz_len + sz_line);

        assert(*md != NULL);

        strncpy(*md + sz_len, line, sz_line);

        sz_len += sz_line;
    }

    return sz_len;
}

ssize_t email_construct(email_t* e)
{
    struct __upload_ctx*    us_ctx;
    ssize_t                 sz_payload;

    us_ctx = e->us_ctx;
    sz_payload = 0;

    strdate( e->time, sizeof(e->time) );

    e->us_ctx = (struct __upload_ctx*)malloc( sizeof(struct __upload_ctx) + 1 );

    us_ctx = e->us_ctx;

    assert(e->us_ctx != NULL);

    us_ctx->lines = 0;
    us_ctx->payload = (char**)malloc( sizeof(char*) * 6 );

    assert(us_ctx->payload != NULL);

    us_ctx->payload[0] = (char*)malloc( sizeof(e->time) + 16 );
    us_ctx->payload[1] = (char*)malloc( sizeof(e->to) + 16 );
    us_ctx->payload[2] = (char*)malloc( sizeof(e->from) + 16 );
    us_ctx->payload[3] = (char*)malloc( sizeof(e->subject) + 16);
    us_ctx->payload[4] = (char*)malloc( strlen(e->body) + 16 );

    for (int i = 0; i < 5; i++)
        assert(us_ctx->payload[i] != NULL);

    sz_payload += sprintf(us_ctx->payload[0], "Date: %s +1000\r\n", e->time);
    sz_payload += sprintf(us_ctx->payload[1], "To: <%s>\r\n", e->to);
    sz_payload += sprintf(us_ctx->payload[2], "From: <%s>\r\n", e->from);
    sz_payload += sprintf(us_ctx->payload[3], "Subject: %s\r\n\r\n", e->subject);
    sz_payload += sprintf(us_ctx->payload[4], "%s\r\n", e->body);

    us_ctx->payload[5] = NULL;

    return sz_payload;
}

CURLcode email_send(email_t* e)
{
    CURL*       curl;
    CURLcode    res;

    curl = curl_easy_init();

    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, "smtp://localhost");
        curl_easy_setopt(curl, CURLOPT_MAIL_FROM, e->from);
        curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, e->csl_recips);
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, __email_payload_read);
        curl_easy_setopt(curl, CURLOPT_READDATA, e);
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

        res = curl_easy_perform(curl);

#if DEBUG
        if (res != CURLE_OK)
            fprintf(stderr,
                    "curl_easy_perform(): %s\n",
                    curl_easy_strerror(res));
#endif

        assert(res == CURLE_OK);
    }

    curl_easy_cleanup(curl);

    return res;
}

int main(int argc, char** argv)
{
    char*   to;
    int     l;
    email_t e;

    if (argc < 2)
    {
        fprintf(stderr,
                "Usage:   %s [Email Recipient]\n" \
                "Example: %s example@gmail.com\n",
                argv[0],
                argv[0]);

        return -1;
    }

    to = argv[1];

    memset( &e, 0, sizeof(email_t) );

    e.csl_recips = curl_slist_append(e.csl_recips, to);

    strncpy( e.to, to, sizeof(e.to) );

    printf("Added recipient address: %s\n\n", to);

    input( "Email from address: ", e.from, sizeof(e.from) );
    input( "Email subject     : ", e.subject, sizeof(e.subject) );

    printf("\n"
           "Please type out your email line by line.\n"
           "When finished, type: %s\n\n",
           INPUT_END);

    input_lines(">", INPUT_END, &e.body);

    email_construct(&e);

    for(int i = 0; i < 35; i++)
        putchar(i == 0 ? '\n' : '-');

    putchar('\n');

    l = 0;

    while (to != NULL)
    {
        to = e.us_ctx->payload[l++];

        printf("%s", to == NULL || l == 0 ? "\n" : to);
    }

    printf("Press ENTER to send or CTRL+C to quit...");
    getchar();

    email_send(&e);

    curl_slist_free_all(e.csl_recips);

    printf("Sent\n");

    return 0;
}
