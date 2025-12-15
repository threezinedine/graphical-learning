import argparse
import os


def run_command(command: str):
    print(f"Running command: {command}")
    os.system(command)


def main():
    parser = argparse.ArgumentParser(
        description="Helper script for Graphical Learning project."
    )

    subparsers = parser.add_subparsers(dest="command")

    subparsers.add_parser("build", help="Build the project.")
    subparsers.add_parser("clean", help="Clean build artifacts.")
    subparsers.add_parser("run", help="Run the project.")

    args = parser.parse_args()

    if args.command == "build":
        run_command("cmake -B build && cmake --build build")
    elif args.command == "clean":
        run_command("rm -rf build")
    elif args.command == "run":
        run_command("cmake --build build && ./build/GraphicalApp")


if __name__ == "__main__":
    main()
