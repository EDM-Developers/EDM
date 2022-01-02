# Chicago crime and temperature example

These examples are described in full in our [paper](pdfs/edm-wp.pdf).
The [code](examples/sj-edm.do) and [data](examples/chicago.dta) can be downloaded directly.
This example look at the causal links between Chicago's temperature and crime rates.
The following example uses synthetic data to do a more advanced analysis.

The package's command keyword is `edm`, and should be immediately followed by the subcommand `explore` or `xmap`.

!!! note
    A dataset must be declared as time-series or panel data by the tsset or xtset command prior to using the edm command, and time-series operators including `l.`, `f.`, `d.`, and `s.` can be used (the last for seasonal differencing).

TODO: Add comments / explanations to the following code snippets.

``` stata
/* Visualising coupling strength using the Chicago crime dataset*/

use chicago,clear

/* Explore the system dimension */
edm explore temp, e(2/20) crossfold(5) seed(1234567)

/* Explore convergance property */
// note this process may take considerable computation time
// it is possible to skip this section if needed
edm xmap temp crime, e(7) library(10(5)200 210(10)1000 1020(20)2000 2050(50)4350 4365) rep(10)
mat cyx= e(xmap_2)
mat cxy= e(xmap_1)
svmat cyx, names(chicago_yx)
svmat cxy, names(chicago_xy)
label variable chicago_xy3 "Crime|M(Temperature)"
label variable chicago_yx3 "Temperature|M(Crime)"
twoway (scatter chicago_xy3 chicago_xy2, mfcolor(%30) mlcolor(%30)) ///
    (scatter chicago_yx3 chicago_yx2, mfcolor(%30) mlcolor(%30)) ///
    (lpoly chicago_xy3 chicago_xy2)(lpoly chicago_yx3 chicago_yx2), xtitle(L) ytitle("{it:{&rho}}") legend(col(1))

graph export chicago-rho-L.pdf, replace

/* save the betas using smap algorithms */

edm xmap temp crime, e(7) alg(smap) k(-1) savesmap(beta)

/* Plot the effects of temperature on crime */
twoway (kdensity beta1_b1_rep1), xtitle("Contemporaneous effect of temperature on crime") ytitle("Density")
graph export chicago-crime1.pdf, replace
twoway (scatter beta1_b1_rep1 temp, xtitle("Temperature (Fahrenheit)") ytitle("Contemporaneous effect of temperature on crime") msize(small))(lpoly beta1_b1_rep1 temp), legend(on order( 1 "Local coefficient" 2 "Local polynomial smoothing"))
graph export chicago-crime2.pdf, replace
```
