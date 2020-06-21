/*
 * main.c: entry point for cccrack
 * Creation date: Mon Feb 25 15:01:52 2019
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>

#include <cccrack.h>

static void
help(const char *progname)
{
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, "  %s [OPTIONS] symbolfile.log\n", progname);
  fprintf(stderr, "\n");
  fprintf(
      stderr,
      "Attempts to blindly guess the parameters of convolutional encoders by\n"
      "by examining a stream of symbols.\n\n");
  fprintf(stderr, "Options:\n");
  fprintf(
      stderr,
      "  -b, --bps=NUM      Force the number of bits per symbol to be NUM\n");
  fprintf(
      stderr,
      "  -t, --tagging=ID   Compute only symbol tagging number ID\n");
  fprintf(
      stderr,
      "  -d, --dump=FILE    Dump retagged input to FILE (use with -t)\n");
  fprintf(
      stderr,
      "  -p, --params=k,n,K Force the parameters of the encoder to be k, n, K\n");
  fprintf(
      stderr,
      "  -n, --no-gray      Show candidates whose tagging is not Gray-coded\n");
  fprintf(
      stderr,
      "  -a, --all          Show all candidates, even the unlikely ones\n");
  fprintf(
      stderr,
      "  -h, --help         This help\n");
  putchar(10);
}

static BOOL
work(
    const char *progname,
    const char *file,
    const struct cccrack_params *params)
{
  unsigned int i, count;
  cccrack_t *cccrack = NULL;
  BOOL ok = FALSE;

  CONSTRUCT(cccrack, cccrack, file, params);

  if (params->tagging == -1)
    fprintf(
        stderr,
        "%s: running on `%s' for all %d different taggings\n",
        progname,
        file,
        cccrack_get_tagging_count(cccrack));

  TRY(cccrack_run(cccrack));

  count = cccrack_get_candidate_count(cccrack);

  if (count == 0) {
    fprintf(stderr, "%s: no candidates found!\n", progname);
    exit(EXIT_FAILURE);
  } else {
    for (i = 0; i < count; ++i)
      cccrack_rankdef_debug(cccrack_get_candidate(cccrack, i));
  }

  ok = TRUE;

fail:
  if (cccrack != NULL)
    cccrack_destroy(cccrack);

  return ok;
}

static struct option long_options[] = {
    {"bps",     required_argument, 0, 'b'},
    {"tagging", required_argument, 0, 't'},
    {"dump",    required_argument, 0, 'd'},
    {"params",  required_argument, 0, 'p'},
    {"no-gray", no_argument,       0, 'n'},
    {"all",     no_argument,       0, 'a'},
    {"help",    no_argument,       0, 'h'},
    {0,         0,                 0,  0 }
};

int
main(int argc, char *argv[], char *envp[])
{
  struct cccrack_params params = cccrack_params_INITIALIZER;
  char *pathdup = NULL;
  int c;
  int digit_optind = 0;
  int this_option_optind;
  int option_index;

  int errcode = EXIT_FAILURE;

  for (;;) {
    this_option_optind = optind ? optind : 1;
    option_index = 0;

    c = getopt_long(
        argc,
        argv,
        "b:t:d:p:nah",
        long_options,
        &option_index);

    if (c == -1)
      break;

    switch (c) {
      case 'b':
        if (sscanf(optarg, "%u", params.bps) < 1) {
          fprintf(stderr, "%s: invalid bps value\n", argv[0]);
          goto fail;
        }

        if (params.bps > 6) {
          fprintf(
              stderr,
              "%s: too many bits per symbol! (max is 6)\n",
              argv[0]);
          goto fail;
        }
        break;

      case 't':
        if (sscanf(optarg, "%d", &params.tagging) < 1) {
          fprintf(stderr, "%s: invalid tagging ID\n", argv[0]);
          goto fail;
        }

        if (params.tagging < 0) {
          fprintf(stderr, "%s: invalid tagging ID\n", argv[0]);
          goto fail;
        }
        break;

      case 'd':
        TRY(pathdup = strdup(optarg));
        params.dumpfile = pathdup;
        break;

      case 'p':
        if (sscanf(optarg, "%u,%u,%u", &params.k, &params.n, &params.K) < 3) {
          fprintf(stderr, "%s: invalid parameters\n", argv[0]);
          goto fail;
        }

        if (params.k >= params.n) {
          fprintf(stderr, "%s: encoder rate is too big\n", argv[0]);
          goto fail;
        }

        if (params.K == 0) {
          fprintf(stderr, "%s: invalid constraint length\n", argv[0]);
          goto fail;
        }
        break;

      case 'a':
        params.all = TRUE;
        break;

      case 'n':
        params.no_gray = TRUE;
        break;

      case '?':
        fprintf(stderr, "%s: unrecognized option `%c'\n", argv[0], optopt);
        help(argv[0]);
        break;

      case ':':
        fprintf(
            stderr,
            "%s: option `%c' expectgs an argument\n",
            argv[0],
            optopt);
        help(argv[0]);
        break;

      default:
        printf("?? getopt returned character code 0%o ??\n", c);
    }
  }

  if (optind == argc) {
    fprintf(stderr, "%s: no files provided\n\n", argv[0]);
    help(argv[0]);
  } else {
    while (optind < argc)
      (void) work(argv[0], argv[optind++], &params);
  }

  errcode = EXIT_SUCCESS;

fail:
  if (pathdup != NULL)
    free(pathdup);

  exit(errcode);
}

