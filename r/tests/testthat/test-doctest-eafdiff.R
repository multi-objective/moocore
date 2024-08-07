# Generated by doctest: do not edit by hand
# Please edit file in R/eafdiff.R

test_that("Doctest: eafdiff", {
  # Created from @doctest for `eafdiff`
  # Source file: R/eafdiff.R
  # Source line: 32
  A1 <- read_datasets(text = "\n 3 2\n 2 3\n\n 2.5 1\n 1 2\n\n 1 2\n")
  A2 <- read_datasets(text = "\n 4 2.5\n 3 3\n 2.5 3.5\n\n 3 3\n 2.5 3.5\n\n 2 1\n")
  d <- eafdiff(A1, A2)
  str(d)
  expect_equal(d, matrix(byrow = TRUE, ncol = 3, scan(quiet = TRUE, text = "1.0  2.0    2\n\n  2.0  1.0   -1\n\n  2.5  1.0    0\n\n  2.0  2.0    1\n\n  2.0  3.0    2\n\n  3.0  2.0    2\n\n  2.5  3.5    0\n\n  3.0  3.0    0\n\n  4.0  2.5    1")))
  expect_true(is.matrix(d))
  d <- eafdiff(A1, A2, rectangles = TRUE)
  str(d)
  d
})

