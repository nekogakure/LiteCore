find src/kernel src/include -type f \( -name "*.c" -o -name "*.h" \) -exec clang-format -i {} +
