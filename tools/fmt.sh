find src/kernel src/include boot -type f \( -name "*.c" -o -name "*.h" \) -exec clang-format -i {} +
