# Compressibility data gathered by Lubomir I Ivanov:
#
#  "Data obtained by finding two books online:
#
#   [1]
#    PERRY’S CHEMICAL ENGINEERS’ HANDBOOK SEVENTH EDITION
#    pretty serious book, from which the wiki AIR values come from!
#
#     http://www.unhas.ac.id/rhiza/arsip/kuliah/Sistem-dan-Tekn-Kendali-Proses/PDF_Collections/REFERENSI/Perrys_Chemical_Engineering_Handbook.pdf
#     page 2-165
#
#     [*](Computed from pressure-volume-temperature tables in Vasserman monographs)
#     ^ i have no idea idea what this means, but the values might not be exactly
#     experimental?!
#
#   the only thing this book is missing is helium, thus [2]!
#
#   [2]
#    VOLUMETRIC BEHAVIOR OF HELIUM-ARGON MIXTURES AT HIGH PRESSURE AND MODERATE TEMPERATURE.
#
#     https://shareok.org/bitstream/handle/11244/2062/6614196.PDF?sequence=1
#     page 108
#
#
#     the book has some tables with pressure values in atmosphere units. i'm
#     converting them bars. one of the relevant tables is for 323K and one for 273K
#     (both almost equal distance from 300K).
#
#     this again is a linear mix operation between isotherms, which is probably not
#     the most accurate solution but it works.
#
# all data sets contain Z values at 300k, while the pressures are in bars in
# the 1 to 500 range
#
#

x  = c(1, 5, 10, 20, 40, 60, 80, 100, 200, 300, 400, 500)
o2 = c(0.9994, 0.9968, 0.9941, 0.9884, 0.9771, 0.9676, 0.9597, 0.9542, 0.9560, 0.9972, 1.0689, 1.1572)
n2 = c(0.9998, 0.9990, 0.9983, 0.9971, 0.9964, 0.9973, 1.0000, 1.0052, 1.0559, 1.1422, 1.2480, 1.3629)
he = c(1.0005, 1.0024, 1.0048, 1.0096, 1.0191, 1.0286, 1.0381, 1.0476, 1.0943, 1.1402, 1.1854, 1.2297)


options(digits=15)

#
# Get the O2 virial coefficients
#
plot(x,o2)
o2fit = nls(o2 ~ 1.0 + p1*x + p2 *x^2 + p3*x^3, start=list(p1=0,p2=0,p3=0))
summary(o2fit)

new = data.frame(x = seq(min(x),max(x),len=200))
lines(new$x,predict(o2fit,newdata=new))

#
# Get the N2 virial coefficients
#
plot(x,n2)
n2fit = nls(n2 ~ 1.0 + p1*x + p2 *x^2 + p3*x^3, start=list(p1=0,p2=0,p3=0))
summary(n2fit)

new = data.frame(x = seq(min(x),max(x),len=200))
lines(new$x,predict(n2fit,newdata=new))

#
# Get the He virial coefficients
#
# NOTE! This will not confirm convergence, thus the warnOnly.
# That may be a sign that the data is possibly artificial.
#
plot(x,he)
hefit = nls(he ~ 1.0 + p1*x + p2 *x^2 + p3*x^3,
	start=list(p1=0,p2=0,p3=0),
	control=nls.control(warnOnly=TRUE))
summary(hefit)

new = data.frame(x = seq(min(x),max(x),len=200))
lines(new$x,predict(hefit,newdata=new))
