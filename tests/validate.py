import os
import argparse

def compare_files(k,path):
    good = True
    for x in range(k):
        file1 = f"{x}.out"
        file2 = f"{x}_f.out"
        try:
            with open(os.path.join(path,file1), "r") as f1, open(os.path.join(path,file2), "r") as f2:
                lines1 = f1.readlines()
                lines2 = f2.readlines()

                if len(lines1) != len(lines2):
                    print(f"Mismatch in number of lines: {len(lines1)} vs {len(lines2)}")
                    continue

                mismatch_found = False
                for i, (line1, line2) in enumerate(zip(lines1, lines2)):
                    # You can strip whitespace if needed:
                    if line1.strip() != line2.strip():
                        print(f"Mismatch at line {i+1}:")
                        print(f"  {file1}: '{line1.strip()}'")
                        print(f"  {file2}: '{line2.strip()}'")
                        mismatch_found = True
                        good = False
                        break  # stop on first mismatch for this file pair

        except FileNotFoundError as e:
            print(f"Error: {e}")
    if good:
        print("No err")
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Validate")
    parser.add_argument("-k", type=int, required=True, help="Number of files to check.")
    parser.add_argument("-p", type=str, required=True, help="Path to directory containing the files.")

    args = parser.parse_args()

    compare_files(args.k, args.p)
