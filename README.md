# Matching Sample

# Purpose
Many people ask, how to create an effective matching engine.
Actually it's not a difficult things, I am here provide an example which can just provide limited order only Matching engine exmaple.

#Environment
Unix Like system only with C++17 support

# Build
Make

#Executalbe
../matching_sample_build/matching_sample/bin/order_book




# API Overriew
## new_order:

auto new_order(TOrder& odr)

modify the odr status and return the matching list


## new_order:

auto cancel_order(TOrder& odr)

modify the odr status


## amend_order(TOrder& odr)


modify the odr status and return the matching list






