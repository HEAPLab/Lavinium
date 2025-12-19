import pandas as pd
import sys
# Define the function to parse the third column
def parse_function_and_passes(value):
  value = value.strip("b'")  # Remove leading b' from byte representation
  value = value.strip("'") # Remove trailing ' from byte representation
  parts = value.split(',')
  function_name = parts[0]
  passes = parts[1].strip().split(' - ') if len(parts) > 1 else []
  return function_name, passes

if __name__ == "__main__":
  # Read the CSV file
  csv_file = sys.argv[1]  

  df = pd.read_csv(csv_file, header=None)
  df.columns = ["id", "benchmark", "function_and_passes", "WCET"]

  # Extract only the benchmark name (remove directory part)
  df["benchmark"] = df["benchmark"].apply(lambda x: x.split("/")[-1])

  df[['function', 'passes']] = df['function_and_passes'].apply(lambda x: pd.Series(parse_function_and_passes(x)))

  df.drop(columns=['function_and_passes'], inplace=True)
      