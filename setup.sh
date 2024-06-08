
CFGDIR="data/meson"

BUILDDIR="$1"
BUILDTYPE="$2"

shift 2

meson setup "$BUILDDIR" \
    --native-file "$CFGDIR/base.ini" \
    --native-file "$CFGDIR/$BUILDTYPE.ini" \
    "$@"
