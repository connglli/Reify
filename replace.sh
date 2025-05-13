for set in {1..3}; do
  for i in {1..10}; do
    sed -i \
      -e 's/bool enableConsistentWalks = true;/bool enableConsistentWalks = false;/' \
      -e 's/const bool enableRandomInitialisations = true;/const bool enableRandomInitialisations = false;/' \
      /local/home/kchopra/coeff_set_"$set"/copy_"$i"/params.hpp
  done
done