#' Read several data sets
#'
#' Reads a text file in table format and creates a matrix from it. The file
#' may contain several sets, separated by empty lines. Lines starting by
#' `'#'` are considered comments and treated as empty lines. The function
#' adds an additional column `set` to indicate to which set each row
#' belongs.
#'
#' @param file `character()`\cr Filename that contains the data.  Each row
#'   of the table appears as one line of the file.  If it does not contain an
#'   \emph{absolute} path, the file name is \emph{relative} to the current
#'   working directory, [base::getwd()].  Tilde-expansion is
#'   performed where supported.  Files compressed with `xz` are supported.
#'
#' @param col_names `character()`\cr Vector of optional names for the variables.  The
#'   default is to use \samp{"V"} followed by the column number.
#'
#' @param text `character()`\cr If `file` is not supplied and this is,
#'   then data are read from the value of `text` via a text connection.
#'   Notice that a literal string can be used to include (small) data sets
#'   within R code.
#'
#' @return  `matrix()`\cr A numerical matrix of the
#'  data in the file. An extra column `set` is added to indicate to
#'  which set each row belongs.
#'
#' @author Manuel \enc{López-Ibáñez}{Lopez-Ibanez}
#'
#' @note There are several examples of data sets in
#'   `system.file(package="moocore","extdata")`.
#'
#' @section Warning:
#'  A known limitation is that the input file must use newline characters
#'  native to the host system, otherwise they will be, possibly silently,
#'  misinterpreted. In GNU/Linux the program `dos2unix` may be used
#'  to fix newline characters.
#'
#'@seealso [utils::read.table()]
#'
#'@examples
#' extdata_path <- system.file(package="moocore","extdata")
#' A1 <- read_datasets(file.path(extdata_path,"ALG_1_dat.xz"))
#' str(A1)
#'
#' read_datasets(text="1 2\n3 4\n\n5 6\n7 8\n", col_names=c("obj1", "obj2"))
#'
#' @keywords file
#' @export
read_datasets <- function(file, col_names, text)
{
  if (missing(file) && !missing(text)) {
    file <- tempfile()
    writeLines(text, file)
    on.exit(unlink(file))
  } else {
    if (!file.exists(file))
      stop("error: ", file, ": No such file or directory");
    file <- normalizePath(file)
    if (endsWith(file, ".xz")) {
      unc_file <- tempfile()
      writeLines(readLines(zz <- xzfile(file, "r")), unc_file)
      close(zz)
      file <- unc_file
      on.exit(unlink(file))
    }
  }
  out <- .Call(R_read_datasets, as.character(file))
  if (missing(col_names))
    col_names <- paste0("V", 1L:(ncol(out)-1))
  colnames(out) <- c(col_names, "set")
  out
}

#' Write data sets
#'
#' Write data sets to a file in the same format as [read_datasets()].
#'
#' @param x `matrix`|`data.frame()`\cr Dataset with at least three
#'   columns, the last one is the set of each point. See also
#'   [read_datasets()].
#'
#' @param file Either a character string naming a file or a connection open for
#'   writing. `""` indicates output to the console.
#'
#' @return No return value, called for side effects
#'@seealso [utils::write.table()], [read_datasets()]
#'
#'@examples
#' x <- read_datasets(text="1 2\n3 4\n\n5 6\n7 8\n", col_names=c("obj1", "obj2"))
#' write_datasets(x)
#'
#' @keywords file
#' @export
write_datasets <- function(x, file = "")
{
  setcol <- ncol(x)
  sets <- x[, setcol]
  nobjs <- setcol - 1L
  x <- x[, 1L:nobjs, drop=FALSE]
  col_names <- colnames(x)
  x <- split.data.frame(x[, 1L:nobjs, drop=FALSE], sets)
  append <- FALSE
  if (!is.null(col_names)) {
    cat(paste("#", paste0(col_names, collapse="\t"), "\n"), file = file, append = append)
    append <- TRUE
  }
  write.table(x[[1]], file = file, row.names=FALSE, col.names=FALSE, append = append)
  x <- tail(x, n=-1)
  for (xi in x) {
    cat("\n", file = file, append = TRUE)
    write.table(xi, file = file, row.names=FALSE, col.names=FALSE, append = TRUE)
  }
}
