% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/hv.R
\name{hv_contributions}
\alias{hv_contributions}
\title{Hypervolume contribution of a set of points}
\usage{
hv_contributions(x, reference, maximise = FALSE)
}
\arguments{
\item{x}{\code{matrix()}|\code{data.frame()}\cr Matrix or data frame of numerical
values, where each row gives the coordinates of a point.}

\item{reference}{\code{numeric()}\cr Reference point as a vector of numerical
values.}

\item{maximise}{\code{logical()}\cr Whether the objectives must be maximised
instead of minimised. Either a single logical value that applies to all
objectives or a vector of logical values, with one value per objective.}
}
\value{
\code{numeric()}\cr A numerical vector
}
\description{
Computes the hypervolume contribution of each point given a set of points
with respect to a given reference point assuming minimization of all
objectives.  Dominated points have zero contribution. Duplicated points have
zero contribution even if not dominated, because removing one of them does
not change the hypervolume dominated by the remaining set.
}
\examples{

data(SPEA2minstoptimeRichmond)
# The second objective must be maximized
# We calculate the hypervolume contribution of each point of the union of all sets.
hv_contributions(SPEA2minstoptimeRichmond[, 1:2], reference = c(250, 0),
            maximise = c(FALSE, TRUE))

# Duplicated points show zero contribution above, even if not
# dominated. However, filter_dominated removes all duplicates except
# one. Hence, there are more points below with nonzero contribution.
hv_contributions(filter_dominated(SPEA2minstoptimeRichmond[, 1:2], maximise = c(FALSE, TRUE)),
                 reference = c(250, 0), maximise = c(FALSE, TRUE))

}
\references{
\insertRef{FonPaqLop06:hypervolume}{moocore}

\insertRef{BeuFonLopPaqVah09:tec}{moocore}
}
\seealso{
\code{\link[=hypervolume]{hypervolume()}}
}
\author{
Manuel \enc{López-Ibáñez}{Lopez-Ibanez}
}
\concept{metrics}
