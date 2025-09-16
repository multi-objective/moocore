source("helper-common.R")

test_that("hypervolume", {

test_hv_file <- function(file, reference, maximise = FALSE) {
  nobj <- length(reference)
  dataset <- if (tools::file_ext(file) == "rds") readRDS(file) else read_datasets(file)
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
  hv_contributions_slow <- function(dataset, reference, maximise)
    hypervolume(dataset, reference, maximise) -
      sapply(1:nrow(dataset), function(x) hypervolume(dataset[-x,], reference, maximise))

  hv_contributions_nondom_slow <- function(dataset, reference, maximise) {
    nondom <- is_nondominated(dataset, maximise = maximise, keep_weakly=TRUE)
    hvc <- numeric(nrow(dataset))
    dataset <- dataset[nondom, , drop=FALSE]
    hvc[nondom] <- hypervolume(dataset, reference, maximise) -
      sapply(1:nrow(dataset), function(x) hypervolume(dataset[-x, , drop=FALSE], reference=reference, maximise=maximise))
    hvc
  }
  reference = c(250,0)
  maximise = c(FALSE,TRUE)
  expect_equal(hv_contributions(SPEA2minstoptimeRichmond[, 1:2], reference = reference, maximise = maximise, ignore_dominated=FALSE),
               hv_contributions_slow(SPEA2minstoptimeRichmond[, 1:2], reference = reference, maximise = maximise))
  expect_equal(hv_contributions(SPEA2minstoptimeRichmond[, 1:2], reference = reference, maximise = maximise),
               hv_contributions_nondom_slow(SPEA2minstoptimeRichmond[, 1:2], reference = reference, maximise = maximise))
})
