argv <- commandArgs(trailingOnly = TRUE)
if (length(argv) != 2L)
  stop("Missing filename")
built_path <- argv[2L]

if (argv[1] == "submit") {
  cli::cat_rule("Submitting", col = "red")
  xfun::submit_cran(built_path)
} else if (argv[1] == "info") {
  size <- file.info(built_path)$size
  cli::cat_rule("Info", col = "cyan")
  cli::cli_inform(c(i = "Path {.file {built_path}}", i = "File size: {prettyunits::pretty_bytes(size)}"))
  cli::cat_line()
} else if (argv[1] == "coverage") {
  Sys.setenv(NOT_CRAN='true')
  options(browser="google-chrome",
    covr.gcov_additional_paths="c",
    covr.filter_non_package=FALSE)
  coverage <- withr::with_makevars(c(MOOCORE_DEBUG='1', LTO_OPT=''),
    covr::package_coverage(type='all', commentDonttest=FALSE, commentDontrun=FALSE, quiet=FALSE, clean=TRUE, pre_clean=TRUE,
      relative_path=normalizePath(file.path(getwd(),".."))))
  print(coverage)
  covr::report(coverage, browse=TRUE)
  covr::to_cobertura(coverage)
} else {
  stop("Unknown command", argv[1])
}
