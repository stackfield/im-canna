Summary: GTK+2 immodule for Canna
Name: im-canna
Version: 0.3.2.2
Release: 1
License: LGPL
Group: Applications/Editors
Source: %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-root
Requires: gtk2 = 2.4.0
BuildPrereq: gtk2-devel

%description
im-canna is an implementation of GTK+2 immodule for Canna.

%prep
%setup -q

%build
%configure
make

%install
rm -rf $RPM_BUILD_ROOT

%makeinstall

%find_lang im-canna

%clean
rm -rf $RPM_BUILD_ROOT

%post
/sbin/ldconfig
%{_bindir}/gtk-query-immodules-2.0 > %{_sysconfdir}/gtk-2.0/gtk.immodules

%postun -p /sbin/ldconfig
%{_bindir}/gtk-query-immodules-2.0 > %{_sysconfdir}/gtk-2.0/gtk.immodules

%files -f im-canna.lang
%defattr(-, root, root)
%{_libdir}/gtk-2.0/2.4.0/immodules/im-canna.la
%{_libdir}/gtk-2.0/2.4.0/immodules/im-canna.so

%changelog
* Tue Jul 27 2004 Yukihiro Nakai <nakai@gnome.gr.jp>
- Add translation.

* Sun May 16 2004 Yukihiro Nakai <nakai@gnome.gr.jp>
- Update for GTK+ 2.4.0

* Sat Oct 18 2003 Yukihiro Nakai <nakai@gnome.gr.jp>
- Path update for gtk+2-2.2.1

* Sat Sep 21 2002 Yukihiro Nakai <nakai@gnome.gr.jp>
- Initial release.
