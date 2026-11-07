cd /Users/pramodkumar/Hack/WS/UIOX
sh 30_KIX/tools/mk_uix_pkg_all.sh \
   50_UIX/21_apps/build/arm64/image/21_apps_ \
   30_KIX/build/arm64/pkg arm64


cd /Users/pramodkumar/Hack/WS/UIOX
./rebuildkernel.sh --continue



cd /Users/pramodkumar/Hack/WS/UIOX
sh 30_KIX/tools/mk_uix_pkg_archs.sh


cd /Users/pramodkumar/Hack/WS/UIOX
sh 30_KIX/tools/mk_uix_pkg_archs.sh


sh 30_KIX/tools/mk_uix_pkg_archs.sh arm64
sh 30_KIX/tools/mk_uix_pkg_archs.sh --no-clean arm64
