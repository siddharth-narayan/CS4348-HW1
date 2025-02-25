for s in {1..1000}; do
  ./parallel_prefix_sum -s "$s"
  if [[ $? -ne 0 ]]; then
    echo "Failure at -s $s"
  fi
done

# for j in {1..64}; do
#   ./parallel_prefix_sum -s 81911 -j "$j"
#   if [[ $? -ne 0 ]]; then
#     echo "Failure at -j $j"
#   fi
# done
