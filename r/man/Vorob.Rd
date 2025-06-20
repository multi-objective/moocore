% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/vorob.R
\name{vorob_t}
\alias{vorob_t}
\alias{vorob_dev}
\title{Vorob'ev computations}
\usage{
vorob_t(x, sets, reference, maximise = FALSE)

vorob_dev(x, sets, reference, ve = NULL, maximise = FALSE)
}
\arguments{
\item{x}{\code{matrix()}|\code{data.frame()}\cr Matrix or data frame of numerical
values that represents multiple sets of points, where each row represents
a point. If \code{sets} is missing, the last column of \code{x} gives the sets.}

\item{sets}{\code{integer()}\cr Vector that indicates the set of each point in
\code{x}. If missing, the last column of \code{x} is used instead.}

\item{reference}{\code{numeric()}\cr Reference point as a vector of numerical
values.}

\item{maximise}{\code{logical()}\cr Whether the objectives must be maximised
instead of minimised. Either a single logical value that applies to all
objectives or a vector of logical values, with one value per objective.}

\item{ve}{\code{matrix()}\cr Vorob'ev expectation, e.g., as returned by \code{\link[=vorob_t]{vorob_t()}}.}
}
\value{
\code{vorob_t} returns a list with elements \code{threshold},
\code{ve}, and \code{avg_hyp} (average hypervolume)

\code{vorob_dev} returns the Vorob'ev deviation.
}
\description{
Compute Vorob'ev threshold, expectation and deviation. Also, displaying the
symmetric deviation function is possible.  The symmetric deviation
function is the probability for a given target in the objective space to
belong to the symmetric difference between the Vorob'ev expectation and a
realization of the (random) attained set.
}
\references{
\insertRef{BinGinRou2015gaupar}{moocore}

C. Chevalier (2013), Fast uncertainty reduction strategies relying on
Gaussian process models, University of Bern, PhD thesis.

\insertRef{Molchanov2005theory}{moocore}
}
\author{
Mickael Binois
}
\concept{eaf}
\examples{
data(CPFs)
res <- vorob_t(CPFs, reference = c(2, 200))
res$threshold
res$avg_hyp
# Now print Vorob'ev deviation
vd <- vorob_dev(CPFs, ve = res$ve, reference = c(2, 200))
vd
}
