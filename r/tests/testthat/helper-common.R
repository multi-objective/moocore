# This file is loaded automatically by testthat (supposedly)
extdata_path <- function(file)
  file.path(system.file(package = "moocore"), "extdata", file)

read_extdata <- function(file) read_datasets(extdata_path(file))

save_csv_xz <- function(code, pattern)
{
  path <- tempfile(fileext = ".csv.xz")
  res <- code
  write.table(file = withr::local_connection(xzfile(path, "wb")),
    res, row.names = FALSE, col.names=FALSE, sep=",")
  path
}

compare_file_text_compressed <- function(old, new)
{
  if (compare_file_binary(old, new))
    return(TRUE)
  old <- base::readLines(withr::local_connection(gzfile(old, open = "rb")), warn = FALSE)
  new <- base::readLines(withr::local_connection(gzfile(new, open = "rb")), warn = FALSE)
  identical(old, new)
}

expect_snapshot_csv_xz <- function(name, code)
{
  # skip_on_ci() # Skip for now until we implement this: https://github.com/tidyverse/ggplot2/blob/main/tests/testthat/helper-vdiffr.R
  name <- paste0(name, ".csv.xz")
  # Announce the file before touching `code`. This way, if `code`
  # unexpectedly fails or skips, testthat will not auto-delete the
  # corresponding snapshot file.
  testthat::announce_snapshot_file(name = name)
  path <- save_csv_xz(code)
  testthat::expect_snapshot_file(path, name = name, compare = compare_file_text_compressed)
}

is_wholenumber <- function(x, tol = .Machine$double.eps^0.5)
  abs(x - round(x)) < tol
