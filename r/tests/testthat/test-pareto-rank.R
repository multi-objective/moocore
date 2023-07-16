source("helper-common.R")

test_that("pareto", {
  test_pareto_rank <- function(extdatafile, maximise = FALSE) {
    data <- read_extdata(extdatafile)
    # Drop set column
    data <- data[,-ncol(data)]
    ranks <- pareto_rank(data, maximise = maximise)
    data2 <- data
    for (r in min(ranks):max(ranks)) {
      # We have to keep_weakly because pareto_rank does the same.
      nondom <- is_nondominated(data2, maximise = maximise, keep_weakly = TRUE)
      expect_equal(data[ranks == r, , drop = FALSE], data2[nondom, , drop = FALSE])
      data2 <- data2[!nondom, , drop = FALSE]
    }
  }
  test_pareto_rank("ALG_2_dat.xz")
  test_pareto_rank("spherical-250-10-3d.txt")
})
