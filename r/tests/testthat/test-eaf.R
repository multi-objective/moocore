source("helper-common.R")

test_that("eaf", {

  test_eaf_dataset <- function(name, percentiles = NULL) {
    dataset <- get(name)
    x <- eaf(dataset, percentiles = percentiles)
    # FIXME: work-around for change in the computation
    x[,3] <- floor(x[,3])
    #saveRDS(x, paste0(name, "-eaf.rds"))
    return(x)
  }
  test_eaf_file <- function(file, percentiles = NULL) {
    dataset <- read_datasets(file)
    x <- eaf(dataset, percentiles = percentiles)
    #saveRDS(x, paste0(basename(file), "-eaf.rds"))
    return(x)
  }
  expect_equal(test_eaf_file(extdata_path("ALG_1_dat.xz")),
               readRDS("ALG_1_dat-eaf.rds"))
  expect_equal(test_eaf_dataset("SPEA2relativeRichmond"),
               readRDS("SPEA2relativeRichmond-eaf.rds"))

  for (i in seq_len(399))
    expect_equal(anyDuplicated(eaf(cbind(0:i, 0:i), sets=0:i)[,1]), 0L)
})

test_that("eafs_sets_non_numeric", {
  x <- matrix(1:10, ncol=2)
  expect_equal(eaf(x, sets=1:5), eaf(x, sets=letters[1:5]))
})
