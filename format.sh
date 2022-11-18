find . -not -path "./[build|\.git]*" | grep '^.*\.[ch]$' | xargs clang-format -style=file --verbose -i 
