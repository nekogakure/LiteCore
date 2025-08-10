#!/bin/bash

# GPG鍵生成用のフラグ
GENERATE_KEY=0

# 引数の処理
for arg in "$@"; do
  if [ "$arg" = "-gk" ]; then
    GENERATE_KEY=1
    shift
  fi
done

MAJOR=$1
MINOR=$2
PATCH=$3

# GPG鍵を生成する場合
if [ $GENERATE_KEY -eq 1 ]; then
  echo "GPG鍵を生成します..."
  cat > /tmp/gpg_key_gen_input << EOF
Key-Type: RSA
Key-Length: 4096
Name-Real: LiteCore Developer
Name-Email: litecore@example.com
Expire-Date: 0
%no-protection
%commit
EOF
  gpg --batch --generate-key /tmp/gpg_key_gen_input
  rm -f /tmp/gpg_key_gen_input
  echo "GPG鍵の生成が完了しました"
  # 鍵を生成した後は通常のリリース処理を続行する
fi

if [ -z "$MAJOR" ] || [ -z "$MINOR" ] || [ -z "$PATCH" ]; then
  if [ -f "version.txt" ]; then
    CURRENT_VERSION=$(cat version.txt)
    IFS='.' read -r MAJOR MINOR PATCH <<< "$CURRENT_VERSION"
    echo "using current version($CURRENT_VERSION)"
  else
    MAJOR=255
    MINOR=255
    PATCH=255
    echo "No version specified and no version.txt found. Using default: $MAJOR.$MINOR.$PATCH"
  fi
fi

VERSION="$MAJOR.$MINOR.$PATCH"
RELEASE_NAME="litecore-$VERSION"
RELEASE_DIR="release"
RELEASE_FILE="$RELEASE_NAME.tar.xz"

# バージョン情報をversion.txtに書き込み
echo "$VERSION" > version.txt
echo "Updated version.txt to $VERSION"

mkdir -p $RELEASE_DIR

mkdir -p /tmp/$RELEASE_NAME

cp -r kernel loader mem scripts LICENSE README.md Makefile make.sh version.txt /tmp/$RELEASE_NAME/

tar -cJf $RELEASE_DIR/$RELEASE_FILE -C /tmp $RELEASE_NAME

# GPG署名処理の改善
if gpg --list-secret-keys > /dev/null 2>&1; then
  echo "GPG key found, signing the release..."
  gpg --detach-sign --armor $RELEASE_DIR/$RELEASE_FILE
  PGP_STATUS="PGP: $RELEASE_DIR/$RELEASE_FILE.asc"
else
  echo "Warning: GPG key is not found, skip signing"
  PGP_STATUS="PGP: skipped (no GPG key found)"
fi

(cd $RELEASE_DIR && sha256sum $RELEASE_FILE > $RELEASE_FILE.sha256)

rm -rf /tmp/$RELEASE_NAME

echo "[SUCCESS] create release $VERSION in $RELEASE_DIR/$RELEASE_FILE"
echo "          $PGP_STATUS"
echo "          SHA256: $RELEASE_DIR/$RELEASE_FILE.sha256"