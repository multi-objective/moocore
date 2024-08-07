source("helper-common.R")

withr::with_output_sink("test-whv.Rout", {

test_that("example:whv_rect", {
rectangles <- as.matrix(read.table(header=FALSE, text='
 1.0  3.0  2.0  Inf    1
 2.0  3.5  2.5  Inf    2
 2.0  3.0  3.0  3.5    3
'))
expect_equal(4, whv_rect (matrix(2, ncol=2), rectangles, reference = 6))
expect_equal(4, whv_rect (matrix(c(2, 1), ncol=2), rectangles, reference = 6))
expect_equal(7, whv_rect (matrix(c(1, 2), ncol=2), rectangles, reference = 6))
expect_equal(26, total_whv_rect (matrix(2, ncol=2), rectangles, reference = 6, ideal = c(1,1)))
expect_equal(30, total_whv_rect (matrix(c(2, 1), ncol=2), rectangles, reference = 6, ideal = c(1,1)))
expect_equal(37.5, total_whv_rect (matrix(c(1, 2), ncol=2), rectangles, reference = 6, ideal = c(1,1)))
})

test_that("whv_rect", {

  rectangles <- as.matrix(read.table(header=FALSE, text=
'1.0  3.0  2.0  Inf    1
1.0  2.0  3.0  3.0    1
2.5  1.0  Inf  2.0    1
2.0  3.5  2.5  Inf    2
2.0  3.0  3.0  3.5    2
3.0  2.5  4.0  3.0    2
3.0  2.0  Inf  2.5    2
4.0  2.5  Inf  3.0    1'))
  x <- matrix(c(2,2), ncol=2)
  expect_equal(whv_rect (x, rectangles, reference = 6), 9.5)

})

test_that("whv2", {

A <- read_datasets(text='
3 2
2 3

2.5 1
1 2
')
B <- read_datasets(text='
4 2.5
3 3
2.5 3.5

3 3
2.5 3.5
')
rectangles <- eafdiff(A,B, rectangles=TRUE)
true_rectangles <- as.matrix(read.table(text='
xmin ymin xmax ymax diff
 1.0  3.0  2.0  Inf    1
 1.0  2.0  3.0  3.0    1
 2.5  1.0  Inf  2.0    1
 2.0  3.5  2.5  Inf    2
 2.0  3.0  3.0  3.5    2
 3.0  2.5  4.0  3.0    2
 3.0  2.0  Inf  2.5    2
 4.0  2.5  Inf  3.0    1', header=TRUE))
expect_equal(rectangles, true_rectangles)
ref <- c(5,5)
ideal <- c(1,1)
whv <- whv_rect(matrix(1,nrow=1,ncol=2), rectangles=rectangles, ref=ref)
expect_equal(whv, 12.5)
whv <- total_whv_rect(matrix(1,nrow=1,ncol=2), rectangles=rectangles, ref=ref, ideal = ideal)
expect_equal(whv, 36)
whv <- whv_rect(matrix(3,nrow=1,ncol=2), rectangles=rectangles, ref=ref)
expect_equal(whv, 0)
whv <- total_whv_rect(matrix(3,nrow=1,ncol=2), rectangles=rectangles, ref=ref, ideal = ideal)
expect_equal(whv, 4)
whv <- total_whv_rect(A[,1:2], rectangles=rectangles, ref=ref, ideal = ideal)
expect_equal(whv, 27.3)
whv <- total_whv_rect(B[,1:2], rectangles=rectangles, ref=ref, ideal = ideal)
expect_equal(whv, 6.05)
})

test_that("whv3", {

A <- read_datasets(text='
1 2
')
B <- read_datasets(text='
2 1
')

rectangles <- eafdiff(A,B, rectangles=TRUE)
ref <- c(3,3)
ideal <- c(1,1)

rects_A <- choose_eafdiff(rectangles, left=TRUE)
rects_B <- choose_eafdiff(rectangles, left=FALSE)

whv <- whv_rect(matrix(1.5,nrow=1,ncol=2), rectangles=rects_A, ref=ref)
expect_equal(whv, 0.5)
whv <- whv_rect(matrix(c(1,2),nrow=1,ncol=2), rectangles=rects_A, ref=ref)
expect_equal(whv, 1)
whv <- whv_rect(matrix(c(1,2),nrow=1,ncol=2), rectangles=rects_B, ref=ref)
expect_equal(whv, 0)
whv <- whv_rect(matrix(c(2,1),nrow=1,ncol=2), rectangles=rects_A, ref=ref)
expect_equal(whv, 0)
whv <- whv_rect(matrix(c(2,1),nrow=1,ncol=2), rectangles=rects_B, ref=ref)
expect_equal(whv, 1)
})

}) # withr::with_output_sink()
