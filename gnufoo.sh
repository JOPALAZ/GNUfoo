#!/bin/bash

data_file=""
settings_file=""
statistic_type=""
n_value=0

# Обработка параметров
while [[ $# -gt 0 ]]; do
  case "$1" in
    -d)
      if [ -z "$statistic_type" ]; then
            statistic_type="$1"
      else 
            echo "Only one flag can be used, --help for help"
            exit 1
      fi
      shift ;;
    -a)
      if [ -z "$statistic_type" ]; then
            statistic_type="$1"
      else 
            echo "Only one flag can be used, --help for help"
            exit 1
      fi
      shift ;;
    -n*)
      n_value="${1#-n}"
      shift ;;
    --help)
    echo  -e "Usage:\n\t-a\t\tFlag for average statistic calculation\n\t-d\t\tFlag for dispersional statistic calculation\n"
    echo  -e "\t-n<value>\tParameter for amount of ticks per stastic record, for example \"-n2000\"\n"
    echo  -e "\t<file1>\t\tFilename of settings file\n\t<file2>\t\tFilename of binary file with input data"
    exit 1
    ;;
    *)
      if [ -z "$data_file" ]; then
        data_file="$1"
      elif [ -z "$settings_file" ]; then
        settings_file="$1"
      else
        echo "Unknown parameter: $1"
        echo "--help for help"
        exit 1
      fi
      shift ;;
  esac
done

# Проверка наличия обязательных параметров
if [ -z "$data_file" ] || [ -z "$settings_file" ]; then
  echo "data or/and settings file parameters weren't passed"
  exit 1
fi
echo -e "set terminal png enhanced notransparent nointerlace truecolor font \"Liberation, 20\" size 2000,1400\nset out \"test_graph.png\"\nset title \"Nadpis Grafu\"\nset xlabel \"Popis osy X\"\nset ylabel \"Popis osy y\"\nplot \"< ./main -d -n1000 < data.dat\" with lines title \"100 náhodných vzorků\"" | gnuplot

# Вывод полученных значений