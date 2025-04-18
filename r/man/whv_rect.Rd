% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/whv.R
\name{whv_rect}
\alias{whv_rect}
\alias{total_whv_rect}
\title{Compute (total) weighted hypervolume given a set of rectangles}
\usage{
whv_rect(x, rectangles, reference, maximise = FALSE)

total_whv_rect(
  x,
  rectangles,
  reference,
  maximise = FALSE,
  ideal = NULL,
  scalefactor = 0.1
)
}
\arguments{
\item{x}{\code{matrix()}|\code{data.frame()}\cr Matrix or data frame of numerical
values, where each row gives the coordinates of a point.}

\item{rectangles}{\code{matrix()}\cr Weighted rectangles that will bias the
computation of the hypervolume. Maybe generated by \code{\link[=eafdiff]{eafdiff()}} with
\code{rectangles=TRUE} or by \code{\link[=choose_eafdiff]{choose_eafdiff()}}.}

\item{reference}{\code{numeric()}\cr Reference point as a vector of numerical
values.}

\item{maximise}{\code{logical()}\cr Whether the objectives must be maximised
instead of minimised. Either a single logical value that applies to all
objectives or a vector of logical values, with one value per objective.}

\item{ideal}{\code{numeric()}\cr Ideal point as a vector of numerical values.  If
\code{NULL}, it is calculated as minimum (or maximum if maximising that
objective) of each objective in the input data.}

\item{scalefactor}{\code{numeric(1)}\cr Real value within \eqn{(0,1]} that scales
the overall weight of the differences. This is parameter psi (\eqn{\psi})
in \citet{DiaLop2020ejor}.}
}
\value{
\code{numeric(1)} A single numerical value.
}
\description{
Calculates the hypervolume weighted by a set of rectangles (with zero weight
outside the rectangles). The function \code{\link[=total_whv_rect]{total_whv_rect()}} calculates the
total weighted hypervolume as \code{\link[=hypervolume]{hypervolume()}}\code{ + scalefactor * abs(prod(reference - ideal)) * whv_rect()}. The details of the computation
are given by \citet{DiaLop2020ejor}.
}
\details{
TODO
}
\examples{
rectangles <- as.matrix(read.table(header=FALSE, text='
 1.0  3.0  2.0  Inf    1
 2.0  3.5  2.5  Inf    2
 2.0  3.0  3.0  3.5    3
'))
whv_rect (matrix(2, ncol=2), rectangles, reference = 6)
whv_rect (matrix(c(2, 1), ncol=2), rectangles, reference = 6)
whv_rect (matrix(c(1, 2), ncol=2), rectangles, reference = 6)

total_whv_rect (matrix(2, ncol=2), rectangles, reference = 6, ideal = c(1,1))
total_whv_rect (matrix(c(2, 1), ncol=2), rectangles, reference = 6, ideal = c(1,1))
total_whv_rect (matrix(c(1, 2), ncol=2), rectangles, reference = 6, ideal = c(1,1))

}
\references{
\insertAllCited{}
}
\seealso{
\code{\link[=read_datasets]{read_datasets()}}, \code{\link[=eafdiff]{eafdiff()}}, \code{\link[=choose_eafdiff]{choose_eafdiff()}}, \code{\link[=whv_hype]{whv_hype()}}
}
\concept{metrics}
