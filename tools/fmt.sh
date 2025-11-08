find src/kernel src/include src/boot -type f \( -name "*.c" -o -name "*.h" \) -exec clang-format -i {} +
