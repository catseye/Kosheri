Kosheri Terms
=============

    -> Functionality "Freeze and Thaw Kosheri Term" is implemented by shell command
    -> "./freeze --termfile %(test-body-file) --binfile foo.bin && ./thaw --binfile foo.bin --termfile %(output-file)"

    -> Tests for functionality "Freeze and Thaw Kosheri Term"

Freeze and thaw a well-formed, complex term.

    | [150, symbol, <tuple: THIS, IS, A, TUPLE > , 611,
    |   <0: 1, <snaaa: 2, <pair: 3, 5>, <singleton: t>, <singleton: t>>>,
    |   <0: 1, 2, 3, <0: 2, 3, <pair: 3, snaaa>, 4, 5>>,
    |   [8, 9, 10, jack, queen | king],
    |  999
    | ]
    = <5: 150, <5: symbol, <5: <tuple: THIS, IS, A, TUPLE>, <5: 611, <5: <0: 1, <snaaa: 2, <pair: 3, 5>, <singleton: t>, <singleton: t>>>, <5: <0: 1, 2, 3, <0: 2, 3, <pair: 3, snaaa>, 4, 5>>, <5: <5: 8, <5: 9, <5: 10, <5: jack, <5: queen, king>>>>>, <5: 999, []>>>>>>>>

    | { dict = wonderful, powerful = 3, nested = { dict = 5, pict = rict }, 7 = quaint }
    = {nested={pict=rict, dict=5}, 7=quaint, powerful=3, dict=wonderful}
