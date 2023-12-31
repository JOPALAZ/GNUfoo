#!/bin/bash

data_file=""
settings_file=""
output_file_default="output.png"
output_set=0
output_file="output.png"

settings_keys=("Referrence Time Stamp - UNIX" "Initial Time Stamp" "Sample Description" "Sampling Freq." "Pre-Amplifier Gain" "Amplifier Gain")
settings_types=("0'st us" "0'st samples" "0'st" "0'st Hz" "0'st dB" "0'st dB")
settings_values=(0 0 0 0 0 0)
got_values=(0 0 0 0 0 0)
statistic_type=""
n_value=0


trim() {
    local str="$*"
    str=$(echo $str |sed 's/^[[:space:]]*//;s/[[:space:]]*$//')
    echo "$str"
}


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
      if [ -z "$n_value" ]; then
        shift
        if [ "$1" -eq "$1" ]; then
          n_value="$1"
        else
          echo "Not all necessary params were given, --help for help"
          exit 1
        fi
      fi
      shift ;;
    --help)
      echo  -e "Usage:\n\t-a\t\tFlag for average statistic calculation\n\t-d\t\tFlag for dispersional statistic calculation\n"
      echo  -e "\t-n<value>\tParameter for amount of ticks per stastic record, for example \"-n2000\"\n"
      echo  -e "\t<file1>\t\tFilename of settings file\n\t<file2>\t\tFilename of binary file with input data\n\t<file3>\t\tGraph image output file"
      exit 1
      ;;
    *)

      if [ -z "$settings_file" ]; then
        settings_file="$1"
      elif [ -z "$data_file" ]; then
        data_file="$1"
      elif [ "$output_file" = "$output_file_default" ] && [ "$output_set" -eq 0 ] ; then
        output_file="$1"
        output_set=1
      else

        echo "Unknown parameter: $1"
        echo "--help for help"
        exit 1
      fi
      shift ;;
  esac
done


if [ -z "$data_file" ] || [ -z "$settings_file" ] || [ -z "$n_value" ] || [ -z "$statistic_type" ]; then
  echo "Not all necessary params were given, --help for help"
  exit 1
fi

if [ -z "$output_file" ]; then
  output_file="$output_file_default"
fi




if [ ! -f "$settings_file" ]; then
  echo "$settings_file can't be read or not found"
  exit 1
fi

while IFS= read -r line; do
  key=$( echo "${line%%":"*}:" | sed 's/^#\(.*\):.*/\1/' )
  key=$(trim $key)
  for index in ${!settings_keys[*]}; do
    if [[ "$key" == ${settings_keys[index]} ]]; then
      prefix=$( echo "$line" | grep -o '#[^:]*:' | head -n1 )
      dirty_value=${line#${prefix}}
      value_number="${settings_types[index]%"'"*}"
      IFS=',' read -a possible_values <<< "$dirty_value"
      settings_values[index]=$(trim $(trim ${possible_values[value_number]} | sed "s/${settings_types[index]##*" "}$//" ))
      got_values[index]=1
    fi
  done
done < "$settings_file"




for val in ${got_values[*]}; do
if [ "$val" -eq 0 ]; then
  echo "Bad settings file"
  exit 1
fi
done









seconds=$(echo "${settings_values[0]} / 1000000" | bc -l)
graph_title=${settings_values[2]}", Ref.Time: "$(date -d "@$seconds" +"%d.%m.%Y %H:%M:%S.%6N")

label_x="Time [ms]"
if [ "$statistic_type" == "-a" ]; then
  label_y="Average[V]"
  line_title="Average 1:""$n_value"
  signal_to_voltage_coefficient="x*(2.048/2**16)*10**-((${settings_values[4]}+${settings_values[5]})/20.0)"
else
  label_y="Dispersion[V^2]"
  line_title="Dispersion 1:""$n_value"
  signal_to_voltage_coefficient="x*((2.048/2**16)*10**-((${settings_values[4]}+${settings_values[5]})/20.0))**2"
fi

gnuplot <<instuctions
sampling_rate = "${settings_values[3]}"
timeshift = ${settings_values[1]}/sampling_rate*1000
calc_time(row) = (row / sampling_rate)*$n_value*1000+timeshift
signal_to_voltage(x) = $signal_to_voltage_coefficient
set autoscale y
set xrange[timeshift:*]
set terminal png enhanced notransparent nointerlace truecolor font "Liberation, 20" size 2000,1400
set out "$output_file"
set title "$graph_title"
set xlabel "$label_x"
set ylabel "$label_y"
plot "< gnufooc $statistic_type -n$n_value < $data_file" using (calc_time(\$0)):(signal_to_voltage(\$1)) with lines title "$line_title"
instuctions

