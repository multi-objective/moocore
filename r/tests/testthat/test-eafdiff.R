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

## FIXME: We need smaller data!
test_that("eafdiff2d", {
  test_eafdiff <- function(a, b, maximise = FALSE, rectangles = FALSE, ...) {
    A1 <- read_datasets(file.path(system.file(package="moocore"), "extdata", a))
    A2 <- read_datasets(file.path(system.file(package="moocore"), "extdata", b))
    minmax <- ""
    if (any(maximise)) {
      minmax <- paste0("-",
        paste0(ifelse(maximise, "max", "min"), collapse="-"))
      A1[, which(maximise)] <- -A1[, which(maximise)]
      A2[, which(maximise)] <- -A2[, which(maximise)]
    }
    # FIXME: It would be much faster to just test the case maximise=FALSE
    # against the snapshot then test that all possible maximise variants are
    # equivalent to maximise=FALSE after negating the appropriate columns.
    a <- tools::file_path_sans_ext(basename(a))
    b <- tools::file_path_sans_ext(basename(b))
    res <- as.data.frame(eafdiff(A1, A2, maximise = maximise, ...))
    expect_true(all(is_wholenumber(res[, ncol(res)])))
    # If we do not save the last column as integer we get spurious differences.
    res[, ncol(res)] <- as.integer(round(res[,ncol(res)]))

    expect_snapshot_csv_xz(paste0("eafdiff-", a, "-", b, minmax, ifelse(rectangles,"-rect","")),
      res)
  }
  test_eafdiff("wrots_l10w100_dat", "wrots_l100w10_dat")
  test_eafdiff("wrots_l10w100_dat", "wrots_l100w10_dat", maximise = c(TRUE, FALSE))
  test_eafdiff("wrots_l10w100_dat", "wrots_l100w10_dat", maximise = c(FALSE, TRUE))
  test_eafdiff("wrots_l10w100_dat", "wrots_l100w10_dat", maximise = c(TRUE, TRUE))
  test_eafdiff("tpls.xz", "rest.xz")
  test_eafdiff("ALG_1_dat.xz", "ALG_2_dat.xz")
})
