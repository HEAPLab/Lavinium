from pathlib import Path

C_DIR = Path("./PolyBench")

def extract_value(parent, name, data):
  splitted_data = data
  start = 0 
  end = 0
  res = []
  pairs = []
  for i, line in enumerate(splitted_data):
    if line.startswith(b"WCET Result Start"):
      start = i+1
    if line.startswith(b"WCET Result End"):
      end = i
      pairs.append((start,end))
  
  if pairs == []:
    raise Exception(f"Malformed result of {name}")
  for start,end in pairs:
    datas = splitted_data[start:end]
    for line in datas:
      split_line = line.split(b":")
      chosed_pass = split_line[0]
      wcet = split_line[1]
      wcet = int(wcet)
      res.append(",".join((f"{parent}/{name}", "\"" + str(chosed_pass) + "\"", str(wcet))) + "\n")
  return res

if __name__ == "__main__":
  benchmarks = [x for x in Path(C_DIR).glob("*/") if x.is_dir()]
  
  with open("results.csv", "w") as f:
    for benchmark in benchmarks:
      name = [x for x in benchmark.glob("*.c")][0].name
      stem = [x for x in benchmark.glob("*.c")][0].stem
      parent = [x for x in benchmark.glob("*.c")][0].parent.stem
      for file in list(benchmark.glob('lavinium_res')):                                       
        reader = open(file, "rb")
        f.writelines(extract_value(parent, name, reader.readlines()))