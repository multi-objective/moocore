.onLoad <- function(lib, pkg){
  Rdpack::Rdpack_bibstyles(package = pkg, authors = "LongNames")
  invisible(NULL)
}
