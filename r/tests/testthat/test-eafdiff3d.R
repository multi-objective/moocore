test_that("eafdiff3d", {
  lin <- read_datasets("lin.S.txt")
  sph <- read_datasets("sph.S.txt")
  setcol <- ncol(lin)
  # This may stop working once we filter uninteresting values in the C code directly.
  DIFF <- eafdiff(lin, sph)
  x <- as.matrix(read.table("lin.S-sph.S-diff.txt.xz", header = FALSE))
  x[, setcol] <- x[, setcol] - x[, setcol+1]
  dimnames(x) <- NULL
  expect_equal(DIFF[, 1:setcol], x[, 1:setcol])
})
