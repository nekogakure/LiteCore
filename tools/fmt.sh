find src/kernel src/user boot -type f \( -name "*.c" -o -name "*.h" \) -exec clang-format -i {} +
