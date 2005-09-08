#!/bin/sh
perl -pi -e 's@\*cc1:\n@$_%(cc1_ssp) @;' \
    $(gcc --print-file specs) &&
perl -pi -e 's@\*cc1plus:\n@$_%(cc1_ssp) @;' \
    $(gcc --print-file specs) &&
echo '*cc1_ssp: 
%{!fno-stack-protector*: -fstack-protector-all}
'    >> $(gcc --print-file specs)
perl -pi -e 's@\*cc1:\n@$_%(cc1_pie) @;' \
    $(gcc --print-file specs) &&
perl -pi -e 's@\*cc1plus:\n@$_%(cc1_pie) @;' \
    $(gcc --print-file specs) &&
perl -pi -e 's@%{pie:-pie}@%(link_pie)@;' \
    $(gcc --print-file specs) &&
perl -pi -e 's@pie:@!no-pie|pie:@g;' $(gcc --print-file specs) &&
perl -pi -e 's@\*cpp:\n@$_%(cpp_pie) @;' $(gcc --print-file specs) &&
echo '*cpp_pie: 
%{!static:%{!no-pie:%{!pie: -D__PIC__ -DPIC}}}
' >> $(gcc --print-file specs) &&
echo '*cc1_pie: 
%{!static:%{!no-pie:%{!pie: -fPIC}}}
' >> $(gcc --print-file specs) &&
echo '*link_pie: 
%{pie:-pie}%{!no-pie:%{!static:%{!Bstatic:%{!i:%{!r: %{!nonow: -z now} %{!norelro: -z relro} %{!shared:%{!Bshareable:%{!pie: -pie}}}}}}}}
' >> $(gcc --print-file specs)
