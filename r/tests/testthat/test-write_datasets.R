source("helper-common.R")

test_that("write_datasets", {
  x <- read_datasets(text="1 2\n3 4\n\n5 6\n7 8\n", col_names=c("obj1", "obj2"))
  y <- read_datasets(text = capture.output(write_datasets(x)),
                     col_names=c("obj1", "obj2"))
  expect_equal(x,y)
})
