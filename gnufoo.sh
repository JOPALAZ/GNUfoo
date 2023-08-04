#!/bin/bash

data_file=""
settings_file=""
output_file_default="output"
output_set=0
output_file="output"

settings_keys=("# Referrence Time Stamp - UNIX:" "# Initial Time Stamp:" "# Sample Description:" "# Sampling Freq.:" "# Pre-Amplifier Gain:" "# Amplifier Gain:")
settings_values=(0 0 0 0 0 0)
got_values=(0 0 0 0 0 0)
gnuplot_instructions=""
statistic_type=""
n_value=0
 
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
  for index in ${!settings_keys[*]}; do
    if [[ "$line" == *${settings_keys[index]}* ]]; then
      if [[ "$line" == *"Description"* ]] || [[ "$line" == *"description"* ]]; then
        settings_values[index]=$(echo "$line" | awk -F ": " '{print $2}')
        if [ ! -z "${settings_values[index]}" ]; then
          got_values[index]=1
        fi
      else
        settings_values[index]=$(echo "$line" | awk -F ": " '{print $2}' | awk '{print $1}')
        if [ ! -z "${settings_values[index]}" ]; then
          got_values[index]=1
        fi
      fi
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

if [ "$statistic_type" == "-a" ]; then
  label_y="Average[V]"
  line_title="Average 1:""$n_value"
  signal_to_voltage_coefficient="x*(2.048/2**16)*10**-((${settings_values[4]}+${settings_values[5]})/20.0)"
else
  label_y="Dispersion[V^2]"
  line_title="Dispersion 1:""$n_value"
  signal_to_voltage_coefficient="x*((2.048/2**16)*10**-((${settings_values[4]}+${settings_values[5]})/20.0))**2"
fi

label_x="Time [ms]"
gnuplot_instructions+="sampling_rate = "${settings_values[3]}"\n"
gnuplot_instructions+="timeshift = ${settings_values[1]}/sampling_rate*1000\n"
gnuplot_instructions+="calc_time(row) = (row / sampling_rate)*$n_value*1000+timeshift\n"
gnuplot_instructions+="signal_to_voltage(x) = $signal_to_voltage_coefficient\n"
gnuplot_instructions+="set autoscale y\n"
gnuplot_instructions+="set xrange[timeshift:*]\n"
gnuplot_instructions+="set terminal png enhanced notransparent nointerlace truecolor font \"Liberation, 20\" size 2000,1400\nset out \"$output_file\"\n"
gnuplot_instructions+="set title \"$graph_title\"\n"
gnuplot_instructions+="set xlabel \"$label_x\"\n"
gnuplot_instructions+="set ylabel \"$label_y\"\n"
gnuplot_instructions+="plot \"< ./gnufooc $statistic_type -n$n_value < $data_file\" using (calc_time(\$0)):(signal_to_voltage(\$1)) with lines title \"$line_title\""


#echo -e "$gnuplot_instructions">debug
echo -e "$gnuplot_instructions" | gnuplot
