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
} else {
  stop("Unknown command")
}
