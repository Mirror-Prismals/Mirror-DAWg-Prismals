import os

def main():
    print("Welcome to DoubleMain!")
    print("Choose an option:")
    print("1. Open dawgserver.py")
    print("2. Open badserver.py")
    print("3. Open main.py")
    choice = input("Enter your choice (1/2/3): ")

    if choice == "1":
        os.system("python MirrorDAWg/dawgserver/dawgserver.py")
    elif choice == "2":
        os.system("python MirrorDAWg/badserver/badserver.py")
    elif choice == "3":
        os.system("python MirrorDAWg/main/main.py")
    else:
        print("Invalid choice. Please enter 1, 2, or 3.")

if __name__ == "__main__":
    main()
