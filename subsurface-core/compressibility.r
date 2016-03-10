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

#
# Raw data from VOLUMETRIC BEHAVIOR OF HELIUM-ARGON MIXTURES [..]
# T=323.15K (50 C)
p323atm = c(674.837, 393.223, 237.310, 146.294, 91.4027, 57.5799, 36.4620, 23.1654, 14.7478, 9.4017, 5.9987, 3.8300,
            540.204, 319.943, 195.008, 120.951, 75.8599, 47.9005, 30.3791, 19.3193, 12.3080, 7.8495, 5.0100, 3.1992)

Hez323 = c(1.28067, 1.16782, 1.10289, 1.06407, 1.04028, 1.02548, 1.01617, 1.01029, 1.00656, 1.00418, 1.00267, 1.00171,
           1.22738, 1.13754, 1.08493, 1.05312, 1.03349, 1.02122, 1.01349, 1.00859, 1.00548, 1.00349, 1.00223, 1.00143)


# T=273.15 (0 C)
p273atm = c(683.599, 391.213, 233.607, 143.091, 89.0521, 55.9640, 35.3851, 22.4593, 14.2908, 9.1072, 5.8095, 3.7083,
            534.047, 312.144, 188.741, 116.508, 72.8529, 45.9194, 29.0883, 18.4851, 11.7702, 7.5040, 4.7881, 3.0570)

Hez273 = c(1.33969, 1.19985, 1.12121, 1.07494, 1.04689, 1.02957, 1.01874, 1.01191, 1.00758, 1.00484, 1.00309, 1.00197,
           1.26914, 1.16070, 1.09837, 1.06118, 1.03843, 1.02429, 1.01541, 1.00980, 1.00625, 1.00398, 1.00254, 1.00162)

p323 = p323atm * 1.01325
p273 = p273atm * 1.01325

x2=append(p323,p273)
he2=append(Hez323,Hez273)

plot(x2,he2)

hefit2 = nls(he2 ~ 1.0 + p1*x2 + p2*x2^2 + p3*x2^3,
        start=list(p1=0,p2=0,p3=0))
summary(hefit2)

he3 = function(bar)
{
  1.0 +0.00047961098687979363 * bar -0.00000004077670019935 * bar^2 +0.00000000000077707035 * bar^3
}

new = data.frame(x2 = seq(min(x2),max(x2),len=200))
lines(new$x2,predict(hefit2,newdata=new))
curve(he3, min(x2),max(x2),add=TRUE)
