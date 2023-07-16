source("helper-common.R")

library(tools) # file_ext
test_that("hypervolume", {

test_hv_file <- function(file, reference, maximise = FALSE) {
  nobj <- length(reference)
  dataset <- if (file_ext(file) == "rds") readRDS(file) else read_datasets(file)
  hypervolume(dataset[,1:nobj], reference = reference, maximise)
}

expect_equal(test_hv_file("DTLZDiscontinuousShape.3d.front.1000pts.10.rds",
  reference = c(10,10,10)),
  719.223555475191)

expect_equal(test_hv_file("duplicated3.inp",
  reference = c(-14324, -14906, -14500, -14654, -14232, -14093)),
  1.52890128312393e+20)

})

test_that("hv_contributions", {
  hv_contributions_slow <- function(dataset, reference, maximise) {
    return(hypervolume(dataset, reference, maximise) -
           sapply(1:nrow(dataset), function(x) hypervolume(dataset[-x,], reference, maximise)))
  }
  reference = c(250,0)
  maximise = c(FALSE,TRUE)
  expect_equal(hv_contributions(SPEA2minstoptimeRichmond[,1:2], reference = reference, maximise = maximise),
               hv_contributions_slow(SPEA2minstoptimeRichmond[,1:2], reference = reference, maximise = maximise))
})
