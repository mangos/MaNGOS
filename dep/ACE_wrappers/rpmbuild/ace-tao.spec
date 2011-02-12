# Set the version number here.
%define ACEVER  5.8.3
%define TAOVER  1.8.3
%define CIAOVER 0.8.3

# Conditional build
# Default values are
#                    --with rnq         (ACE_HAS_REACTOR_NOTIFICATION_QUEUE)
#                    --with ipv6        (IPv6 support)
#                    --with opt         (Optimized build)
#                    --with zlib        (Zlib compressor)
#                    --with bzip2       (Bzip2 compressor)
#                    --without autoconf (Use MPC to build)
#                    --without fltk     (No ftlk support)
#                    --without tk       (No tk support)
#                    --without xt       (No xt support)
#                    --without fox      (No fox support)
#                    --without qt       (No qt support)

#
# Read: If neither macro exists, then add the default definition.
%{!?_with_rnq: %{!?_without_rnq: %define _with_rnq --with-rnq}}
%{!?_with_ipv6: %{!?_without_ipv6: %define _with_ipv6 --with-ipv6}}
%{!?_with_opt: %{!?_without_opt: %define _with_opt --with-opt}}
%{!?_with_zlib: %{!?_without_zlib: %define _with_zlib --with-zlib}}
%{!?_with_bzip2: %{!?_without_bzip2: %define _with_bzip2 --with-bzip2}}
%{!?_with_autoconf: %{!?_without_autoconf: %define _without_autoconf --without-autoconf}}
%{!?_with_ftlk: %{!?_without_ftlk: %define _without_ftlk --without-ftlk}}
%{!?_with_tk: %{!?_without_tk: %define _without_tk --without-tk}}
%{!?_with_xt: %{!?_without_xt: %define _without_xt --without-xt}}
%{!?_with_fox: %{!?_without_fox: %define _without_fox --without-fox}}
%{!?_with_qt: %{!?_without_qt: %define _without_qt --without-qt}}
#
# Read: It's an error if both or neither required options exist.
%{?_with_rnq: %{?_without_rnq: %{error: both _with_rnq and _without_rnq}}}
%{?_with_ipv6: %{?_without_ipv6: %{error: both _with_ipv6 and _without_ipv6}}}
%{?_with_opt: %{?_without_opt: %{error: both _with_opt and _without_opt}}}
%{?_with_zlib: %{?_without_zlib: %{error: both _with_zlib and _without_zlib}}}
%{?_with_bzip2: %{?_without_bzip2: %{error: both _with_bzip2 and _without_bzip2}}}
%{?_with_autoconf: %{?_without_autoconf: %{error: both _with_autoconf and _without_autoconf}}}
%{?_with_fltk: %{?_without_fltk: %{error: both _with_fltk and _without_fltk}}}
%{?_with_tk: %{?_without_tk: %{error: both _with_tk and _without_tk}}}
%{?_with_xt: %{?_without_xt: %{error: both _with_xt and _without_xt}}}
%{?_with_fox: %{?_without_fox: %{error: both _with_fox and _without_fox}}}
%{?_with_qt: %{?_without_qt: %{error: both _with_qt and _without_qt}}}

%{!?skip_make:%define skip_make 0}
%{!?make_nosrc:%define make_nosrc 0}

%define have_fox 0

%if %{?_with_opt:0}%{!?_with_opt:1}
%define OPTTAG .O0
%endif

Summary:      The ADAPTIVE Communication Environment (ACE) and The ACE ORB (TAO)
Name:         ace-tao
Version:      %{ACEVER}

%if 0%{?opensuse_bs}
Release:      <CI_CNT>%{?OPTTAG}%{?dist}
%else
Release:      1%{?OPTTAG}%{?dist}
%endif

Group:        Development/Libraries/C and C++
URL:          http://www.cs.wustl.edu/~schmidt/ACE.html
License:      DOC License
Source0:      http://download.dre.vanderbilt.edu/previous_versions/ACE+TAO+CIAO-src-%{ACEVER}.tar.gz
Source1:      ace-tao-rpmlintrc
BuildRoot:    %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

%if 0%{?fedora} || 0%{?rhel_version} || 0%{?centos_version}
%define _extension .gz
BuildRequires: redhat-rpm-config elfutils sendmail
%endif

%if 0%{?suse_version}
%define _extension .gz
%endif

%if !0%{?suse_version}
Requires(post):   /sbin/install-info
Requires(preun):  /sbin/install-info
Requires(postun): /sbin/ldconfig
%else
PreReq:         %install_info_prereq %insserv_prereq  %fillup_prereq
PreReq:         pwdutils
%endif

%if 0%{?mdkversion}
BuildRequires:  sendmail
%endif

BuildRequires:  openssl-devel
BuildRequires:  gcc-c++
BuildRequires:  libstdc++-devel
BuildRequires:  lsb

%if %{?_with_zlib:1}%{!?_with_zlib:0}
BuildRequires:  zlib-devel
%endif

%if %{?_with_bzip2:1}%{!?_with_bzip2:0}
BuildRequires:  bzip2
%endif

BuildRequires:  perl

%if %{?_with_fltk:1}%{!?_with_fltk:0}
BuildRequires:  fltk-devel
%define fltk_pac ace-flreactor
%endif

%if %{?_with_tk:1}%{!?_with_tk:0}
BuildRequires:  tcl-devel
BuildRequires:  tk-devel
BuildRequires:  tk
%define tk_pac ace-tkreactor
%define tao_tk_pac tao-tkresource
%endif

%if %{?_with_qt:1}%{!?_with_qt:0}
%define qt_pack ace-qtreactor
%define tao_qt_pac tao-qtresource

# qt3 has a name change in F9
%if 0%{?fedora} > 8
%define qtpacname qt3
%else
%define qtpacname qt
%endif

%if 0%{?suse_version}
%define qtpacname qt3
%endif

BuildRequires:  %{qtpacname}-devel
%endif

%if %{?_with_fox:1}%{!?_with_fox:0}
%if 0%{?suse_version} == 1020
BuildRequires: fox16-devel
%endif
%define fox_pac ace_foxreactor
%endif

%if %{?_with_xt:1}%{!?_with_xt:0}
# The xorg package naming scheme changed, use specific files for now.
# old -> BuildRequires: xorg-x11-devel
# new -> BuildRequires: libX11-devel
# BuildRequires: %{_libdir}/libX11.so
# BuildRequires: %{_libdir}/libXt.so
%define xt_pac ace-xtreactor
%define tao_xt_pac tao-xtresource
%endif

%if %{?_with_fl:1}%{!?_with_fl:0}
%define tao_fl_pac tao-flresource
%endif

%if 0%{?suse_version}
%define ace_packages ace ace-xml ace-gperf ace-kokyu
%define tao_packages tao tao-utils tao tao-cosnaming tao-cosevent tao-cosnotification tao-costrading tao-rtevent tao-cosconcurrency
%define all_ace_packages %{?ace_packages} %{?fltk_pac} %{?tk_pac} %{?qt_pac} %{?fox_pac} %{?xt_pac}
%define all_tao_packages %{?tao_packages} %{?tao_fl_pac} %{?tao_qt_pac} %{?tao_xt_pac} %{?tao_tk_pac}
%define debug_package_requires %{all_ace_packages} %{all_tao_packages}
%endif

%if %make_nosrc
# Leave out the distro for now
NoSource: 0
%endif

%description -n ace-tao

The ADAPTIVE Communication Environment (ACE) is a freely available,
open-source object-oriented (OO) framework that implements many core
patterns for concurrent communication software. ACE provides a rich
set of reusable C++ wrapper facades and framework components that
perform common communication software tasks across a range of OS
platforms. The communication software tasks provided by ACE include
event demultiplexing and event handler dispatching, signal handling,
service initialization, interprocess communication, shared memory
management, message routing, dynamic (re)configuration of distributed
services, concurrent execution and synchronization.

TAO is a real-time implementation of CORBA built using the framework
components and patterns provided by ACE. TAO contains the network
interface, OS, communication protocol, and CORBA middleware components
and features. TAO is based on the standard OMG CORBA reference model,
with the enhancements designed to overcome the shortcomings of
conventional ORBs for high-performance and real-time applications.

# ---------------- ace ----------------

%package -n     ace
Summary:        The ADAPTIVE Communication Environment (ACE)
Version:        %{ACEVER}
Group:          Development/Libraries/C and C++
Requires:       openssl

%description -n ace

The ADAPTIVE Communication Environment (ACE) is a freely available,
open-source object-oriented (OO) framework that implements many core
patterns for concurrent communication software. ACE provides a rich
set of reusable C++ wrapper facades and framework components that
perform common communication software tasks across a range of OS
platforms. The communication software tasks provided by ACE include
event demultiplexing and event handler dispatching, signal handling,
service initialization, interprocess communication, shared memory
management, message routing, dynamic (re)configuration of distributed
services, concurrent execution and synchronization.

# ---------------- ace-devel ----------------

%package -n     ace-devel
Summary:        Header files and development components for ACE
Version:        %{ACEVER}
Group:          Development/Libraries/C and C++
Requires:       ace = %{ACEVER}
Requires:       openssl-devel
%if !0%{?suse_version}
Provides:       perl(PerlACE::Run_Test) perl(Process) perl(VmsProcess) perl(Win32::Process)
%endif

%description -n ace-devel

This package contains the components needed for developing programs
using ACE.

# ---------------- ace-xml ----------------

%package -n     ace-xml
Summary:        ACE XML Runtime Support
Version:        %{ACEVER}
Group:          Development/Libraries/C and C++
Requires:       ace = %{ACEVER}

%description -n ace-xml

ACE XML Parser interfaces follows the the design of SAX 2.0, which is
a public domain specification for Java.  The major difference between
ACE XML Parser interfaces and SAX is that we added an reference of
ACEXML_Env to every SAX method to accommodate platforms/compilers that
don't support C++ exceptions.  SAX is defined by David Megginson
<david@megginson.com>

# ---------------- ace-gperf ----------------

%package -n     ace-gperf
Summary:        ACE gperf
Version:        %{ACEVER}
Group:          Development/Libraries/C and C++
Requires:       ace = %{ACEVER}

%description -n ace-gperf

ACE gperf utility

# ---------------- ace-xml-devel ----------------

%package -n     ace-xml-devel
Summary:        Header files and development components for ACE XML
Version:        %{ACEVER}
Group:          Development/Libraries/C and C++
Requires:       ace-devel = %{ACEVER}
Requires:       ace-xml = %{ACEVER}

%description -n ace-xml-devel

This package contains the components needed for developing programs
using ACEXML.

# ---------------- ace-kokyu ----------------

%package -n     ace-kokyu
Summary:        Kokyu scheduling framework for ACE
Version:        %{ACEVER}
Group:          Development/Libraries/C and C++
Requires:       ace = %{ACEVER}

%description -n ace-kokyu

Kokyu is a portable middleware scheduling framework designed to
provide flexible scheduling and dispatching services within the
context of higher-level middleware. Kokyu currently provides real-time
scheduling and dispatching services for TAO's real-time Event Service
which mediates supplier-consumer relationships between application
operations. Kokyu also provides a scheduling and dispatching framework
for threads. This is being used by the TAO RTCORBA 2.0 scheduler
implementations.

# ---------------- ace-kokyu-devel ----------------

%package -n     ace-kokyu-devel
Summary:        Header files and development components for the ACE Kokyu scheduler
Version:        %{ACEVER}
Group:          Development/Libraries/C and C++
Requires:       ace-devel = %{ACEVER}
Requires:       ace-kokyu = %{ACEVER}

%description -n ace-kokyu-devel

This package contains the components needed for developing programs
using Kokyu.


# ---------------- ace-foxreactor ----------------

%if %{?_with_fox:1}%{!?_with_fox:0}
%if 0%{?have_fox} == 1
%package -n     ace-foxreactor
Summary:        ACE_FoxReactor for use with the FOX toolkit
Version:        %{ACEVER}
Group:          Development/Libraries/C and C++
Requires:       ace = %{ACEVER}
Requires:       fox16

%description -n ace-foxreactor

A Reactor implementation that uses the FOX toolkit for
event demultiplexing.  This will let us integrate the FOX toolkit with
ACE and/or TAO.
%endif
%endif

# ---------------- ace-foxreactor-devel ----------------

%if %{?_with_fox:1}%{!?_with_fox:0}
%if 0%{?have_fox} == 1
%package -n     ace-foxreactor-devel
Summary:        Header files for development with ACE_FoxReactor
Version:        %{ACEVER}
Group:          Development/Libraries/C and C++
Requires:       ace-devel = %{ACEVER}
Requires:       ace-foxreactor = %{ACEVER}
Requires:       fox16-devel

%description -n ace-foxreactor-devel

This package contains the components needed for developing programs
using the ACE_FoxReactor.
%endif
%endif

# ---------------- ace-flreactor ----------------

%if %{?_with_fl:1}%{!?_with_fl:0}
%package -n     ace-flreactor
Summary:        ACE_FlReactor for use with the Fast-Light toolkit
Version:        %{ACEVER}
Group:          Development/Libraries/C and C++
Requires:       ace = %{ACEVER}
Requires:       fltk

%description -n ace-flreactor

A Reactor implementation that uses the Fast-Light (FL) toolkit for
event demultiplexing.  This will let us integrate the FL toolkit with
ACE and/or TAO.
%endif

# ---------------- ace-flreactor-devel ----------------

%if %{?_with_fl:1}%{!?_with_fl:0}
%package -n     ace-flreactor-devel
Summary:        Header files for development with ACE_FlReactor
Version:        %{ACEVER}
Group:          Development/Libraries/C and C++
Requires:       ace-devel = %{ACEVER}
Requires:       ace-flreactor = %{ACEVER}
Requires:       fltk-devel

%description -n ace-flreactor-devel

This package contains the components needed for developing programs
using the ACE_FlReactor.
%endif

# ---------------- ace-qtreactor ----------------

%if %{?_with_qt:1}%{!?_with_qt:0}
%package -n     ace-qtreactor
Summary:        ACE_QtReactor for use with Qt library
Version:        %{ACEVER}
Group:          Development/Libraries/C and C++
Requires:       ace = %{ACEVER}
Requires:       qt

%description -n ace-qtreactor

A Reactor implementation that uses the Qt toolkit for event
demultiplexing.  This will let us integrate the Qt toolkit with ACE
and/or TAO.
%endif

# ---------------- ace-qtreactor-devel ----------------

%if %{?_with_qt:1}%{!?_with_qt:0}
%package -n     ace-qtreactor-devel
Summary:        Header files for development with ACE_QtReactor
Version:        %{ACEVER}
Group:          Development/Libraries/C and C++
Requires:       ace-devel = %{ACEVER}
Requires:       ace-qtreactor = %{ACEVER}
Requires:       qt-devel

%description -n ace-qtreactor-devel

This package contains the components needed for developing programs
using the ACE_QtReactor.
%endif

# ---------------- ace-tkreactor ----------------

%if %{?_with_tk:1}%{!?_with_tk:0}
%package -n     ace-tkreactor
Summary:        ACE_TkReactor for use with Tk toolkit
Version:        %{ACEVER}
Group:          Development/Libraries/C and C++
Requires:       ace = %{ACEVER}
Requires:       tk

%description -n ace-tkreactor

A Reactor implementation that uses the Tk toolkit for event
demultiplexing.  This will let us integrate the Tk toolkit with ACE
and/or TAO.
%endif

# ---------------- ace-tkreactor-devel ----------------

%if %{?_with_tk:1}%{!?_with_tk:0}
%package -n     ace-tkreactor-devel
Summary:        Header files for development with ACE_TkReactor
Version:        %{ACEVER}
Group:          Development/Libraries/C and C++
Requires:       ace-devel = %{ACEVER}
Requires:       ace-tkreactor = %{ACEVER}
Requires:       tk-devel

%description -n ace-tkreactor-devel

This package contains the components needed for developing programs
using the ACE_TkReactor.
%endif

# ---------------- ace-xtreactor ----------------

%if %{?_with_xt:1}%{!?_with_xt:0}
%package -n     ace-xtreactor
Summary:        ACE_XtReactor for use with the X Toolkit
Version:        %{ACEVER}
Group:          Development/Libraries/C and C++
Requires:       ace = %{ACEVER}
# The xorg packaging scheme changed, let autoreq to the job for now.
# Requires: xorg-x11-libs

%description -n ace-xtreactor

A Reactor implementation that uses the X Toolkit for event
demultiplexing.  This will let us integrate the X Toolkit with ACE
and/or TAO.
%endif

# ---------------- ace-xtreactor-devel ----------------

%if %{?_with_xt:1}%{!?_with_xt:0}
%package -n     ace-xtreactor-devel
Summary:        Header files for development with ACE_XtReactor
Version:        %{ACEVER}
Group:          Development/Libraries/C and C++
Requires:       ace-devel = %{ACEVER}
Requires:       ace-xtreactor = %{ACEVER}
# The xorg package naming scheme changed, use specific files for now.
# old -> Requires: xorg-x11-devel
# new -> Requires: libX11-devel
Requires: %{_libdir}/libX11.so
Requires: %{_libdir}/libXt.so

%description -n ace-xtreactor-devel

This package contains the components needed for developing programs
using the ACE_XtReactor.
%endif

# ---------------- MPC ----------------

%package -n   mpc
Summary:      Make Project Creator
Version:      %{ACEVER}
Group:        Development/Tools/Building
%if !0%{?suse_version}
Provides:     perl(Driver) perl(MakeProjectBase) perl(ObjectGenerator) perl(ProjectCreator) perl(WorkspaceCreator) perl(WorkspaceHelper) perl(DependencyWriter) perl(WIXProjectCreator)
%endif

%description -n mpc

The Makefile, Project and Workspace Creator.
Designed by Justin Michel (michel_j@ociweb.com) and Chad Elliott.
Implemented by Chad Elliott (elliott_c@ociweb.com).

A single tool (MPC) can be used to generate tool specific input (i.e.
Makefile, dsp, vcproj, etc). The generator takes platform and building
tool generic files (mpc files) as input which describe basic information
needed to generate a "project" file for various build tools. These tools
include Make, NMake, Visual C++ 6, Visual C++ 7, etc.

# ---------------- tao ----------------

%package -n     tao
Summary:        The ACE ORB (TAO)
Version:        %{TAOVER}
Group:          Development/Libraries/C and C++
Requires:       ace = %{ACEVER}

%description -n tao

TAO is a real-time implementation of CORBA built using the framework
components and patterns provided by ACE. TAO contains the network
interface, OS, communication protocol, and CORBA middleware components
and features. TAO is based on the standard OMG CORBA reference model,
with the enhancements designed to overcome the shortcomings of
conventional ORBs for high-performance and real-time applications.

# ---------------- tao-devel ----------------

%package -n     tao-devel
Summary:        Header files and development components for TAO
Version:        %{TAOVER}
Group:          Development/Libraries/C and C++
Requires:       tao = %{TAOVER}
Requires:       ace-devel = %{ACEVER}
Requires:       ace-gperf = %{ACEVER}

%description -n tao-devel

This package contains the components needed for developing programs
using TAO.

# ---------------- tao-utils ----------------

%package -n     tao-utils
Summary:        TAO naming service and IOR utilities
Version:        %{TAOVER}
Group:          Development/Libraries/C and C++
Requires:       tao = %{TAOVER}

%description -n tao-utils

This package includes programs to query or control a CORBA naming service,
and to dump an IOR.

The following programs are included:
* tao-nslist, to list naming context and object bindings
* tao-nsadd, to create bindings
* tao-nsdel, to remove bindings
* tao-catior, to dump the content of an Interoperable Object Reference

# ---------------- tao-cosnaming ----------------

%package -n     tao-cosnaming
Summary:        The TAO CORBA Naming Service (CosNaming) and Interoperable Naming Service (INS)
Version:        %{TAOVER}
Group:          Development/Libraries/C and C++
Requires:       tao = %{TAOVER}
Requires:       logrotate

%description -n tao-cosnaming

OMG defined CORBA Naming Service to provide a basic service location
mechanism for CORBA systems. CosNaming manages a hierarchy of
name-to-object-reference mappings. Anything, but typically the server
process hosting an object, may bind an object reference with a name in
the Naming Service by providing the name and object
reference. Interested parties (typically clients) can then use the
Naming Service to resolve a name to an object reference.

More recently, CORBA Naming Service was subsumed/extended by the CORBA
Interoperable Naming Service, a.k.a. INS. INS inherits all the
functionality from the original Naming Service specification in
addition to addressing some its shortcomings. In particular, INS
defines a standard way for clients and servers to locate the Naming
Service itself. It also allows the ORB to be administratively
configured for bootstrapping to services not set up with the orb at
install time.

# ---------------- tao-cosevent ----------------

%package -n     tao-cosevent
Summary:        The TAO CORBA CosEvent Service
Version:        %{TAOVER}
Group:          Development/Libraries/C and C++
Requires:       tao = %{TAOVER}
Requires:       logrotate

%description -n tao-cosevent

The CosEvent_Service is a COS compliant Event Service.

The service is registered with the naming service with the name
"CosEventService" . It exposes the <EventChannel> interface which can be
used by suppliers and consumers to send and receive events.

# ---------------- tao-cosnotification ----------------

%package -n     tao-cosnotification
Summary:        The TAO CORBA Notification Service
Version:        %{TAOVER}
Group:          Development/Libraries/C and C++
Requires:       tao = %{TAOVER}
Requires:       logrotate

%description -n tao-cosnotification

The Notify_Service is a COS compliant Notification Service.

The Notify_Service executable starts up a Notification Service factory
and registers it with the Naming Service under the name
"NotifyEventChannelFactory"

# ---------------- tao-costrading ----------------

%package -n     tao-costrading
Summary:        The TAO CORBA Trading Service
Version:        %{TAOVER}
Group:          Development/Libraries/C and C++
Requires:       tao = %{TAOVER}
Requires:       logrotate

%description -n tao-costrading

The Trading_Service is a COS compliant Trading Service.

# ---------------- tao-rtevent ----------------

%package -n     tao-rtevent
Summary:        The TAO Real-time Event Service
Version:        %{TAOVER}
Group:          Development/Libraries/C and C++
Requires:       tao = %{TAOVER}
Requires:       logrotate

%description -n tao-rtevent

The TAO Real-Time Event Service. This is a TAO specific service
implementation

# ---------------- tao-cosconcurrency ----------------

%package -n     tao-cosconcurrency
Summary:        The TAO CORBA Concurrency Service
Version:        %{TAOVER}
Group:          Development/Libraries/C and C++
Requires:       tao = %{TAOVER}
Requires:       logrotate

%description -n tao-cosconcurrency

The CORBA Concurrency Service. One of the standard CORBA services.

# ---------------- tao-flresource ----------------

%if %{?_with_fl:1}%{!?_with_fl:0}
%package -n     tao-flresource
Summary:        FlResource_Factory for creating FlReactor
Version:        %{TAOVER}
Group:          Development/Libraries/C and C++
Requires:       ace-flreactor = %{ACEVER}
Requires:       tao = %{TAOVER}

%description -n tao-flresource

This factory is intended for creating FlReactor for ORB. This factory
can be feed into ORB using TAO_ORB_Core::set_gui_resource_factory
method which is usually done by TAO_FlResource_Loader.
%endif

# ---------------- tao-flresource-devel ----------------

%if %{?_with_fl:1}%{!?_with_fl:0}
%package -n     tao-flresource-devel
Summary:        Header files for development with FlResource_Factory
Version:        %{TAOVER}
Group:          Development/Libraries/C and C++
Requires:       ace-flreactor-devel = %{ACEVER}
Requires:       tao-devel = %{TAOVER}
Requires:       tao-flresource = %{TAOVER}

%description -n tao-flresource-devel

This package contains the components needed for developing programs
using the FlResource_Factory.
%endif

# ---------------- tao-qtresource ----------------

%if %{?_with_qt:1}%{!?_with_qt:0}
%package -n     tao-qtresource
Summary:        QtResource_Factory for creating QtReactor
Version:        %{TAOVER}
Group:          Development/Libraries/C and C++
Requires:       ace-qtreactor = %{ACEVER}
Requires:       tao = %{TAOVER}

%description -n tao-qtresource

This factory is intended for creating QtReactor for ORB. This factory
can be feed into ORB using TAO_ORB_Core::set_gui_resource_factory
method which is usually done by TAO_QtResource_Loader.
%endif

# ---------------- tao-qtresource-devel ----------------

%if %{?_with_qt:1}%{!?_with_qt:0}
%package -n     tao-qtresource-devel
Summary:        Header files for development with QtResource_Factory
Version:        %{TAOVER}
Group:          Development/Libraries/C and C++
Requires:       ace-qtreactor-devel = %{ACEVER}
Requires:       tao-devel = %{TAOVER}
Requires:       tao-qtresource = %{TAOVER}

%description -n tao-qtresource-devel

This package contains the components needed for developing programs
using the QtResource_Factory.
%endif

# ---------------- tao-tkresource ----------------

%if %{?_with_tk:1}%{!?_with_tk:0}
%package -n     tao-tkresource
Summary:        TkResource_Factory for creating TkReactor
Version:        %{TAOVER}
Group:          Development/Libraries/C and C++
Requires:       ace-tkreactor = %{ACEVER}
Requires:       tao = %{TAOVER}

%description -n tao-tkresource

This factory is intended for creating TkReactor for ORB. This factory
can be feed into ORB using TAO_ORB_Core::set_gui_resource_factory
method which is usually done by TAO_TkResource_Loader.
%endif

# ---------------- tao-tkresource-devel ----------------

%if %{?_with_tk:1}%{!?_with_tk:0}
%package -n     tao-tkresource-devel
Summary:        Header files for development with TkResource_Factory
Version:        %{TAOVER}
Group:          Development/Libraries/C and C++
Requires:       ace-tkreactor-devel = %{ACEVER}
Requires:       tao-devel = %{TAOVER}
Requires:       tao-tkresource = %{TAOVER}

%description -n tao-tkresource-devel

This package contains the components needed for developing programs
using the TkResource_Factory.
%endif

# ---------------- tao-xtresource ----------------

%if %{?_with_xt:1}%{!?_with_xt:0}
%package -n     tao-xtresource
Summary:        XtResource_Factory for creating XtReactor
Version:        %{TAOVER}
Group:          Development/Libraries/C and C++
Requires:       ace-xtreactor = %{ACEVER}
Requires:       tao = %{TAOVER}

%description -n tao-xtresource

This factory is intended for creating XtReactor for ORB. This factory
can be feed into ORB using TAO_ORB_Core::set_gui_resource_factory
method which is usually done by TAO_XtResource_Loader.
%endif

# ---------------- tao-xtresource-devel ----------------

%if %{?_with_xt:1}%{!?_with_xt:0}
%package -n     tao-xtresource-devel
Summary:        Header files for development with XtResource_Factory
Version:        %{TAOVER}
Group:          Development/Libraries/C and C++
Requires:       ace-xtreactor-devel = %{ACEVER}
Requires:       tao-devel = %{TAOVER}
Requires:       tao-xtresource = %{TAOVER}

%description -n tao-xtresource-devel

This package contains the components needed for developing programs
using the XtResource_Factory.
%endif

# ================================================================
# prep
# ================================================================

%prep
%setup -q -n ACE_wrappers

# ================================================================
# build
# ================================================================

%build

export ACE_ROOT=$(pwd)
export TAO_ROOT=$ACE_ROOT/TAO
export CIAO_ROOT=$TAO_ROOT/CIAO
export DANCE_ROOT=$CIAO_ROOT/DAnCE
export LD_LIBRARY_PATH=$ACE_ROOT/lib

# Dump the g++ versions, in case the g++ version is broken we can
# easily see this in the build log
g++ --version
g++ -dumpversion

%if %skip_make

cd .. && rm -rf ACE_wrappers && ln -s ACE_wrappers-BUILT ACE_wrappers

%else

%if %{?_with_autoconf:0}%{!?_with_autoconf:1}

cat > $ACE_ROOT/ace/config.h << EOF
EOF

# If ipv6 support is indicated insert some lines into the config.h file
#
%if %{?_with_ipv6:1}%{!?_with_ipv6:0}
cat >> $ACE_ROOT/ace/config.h << EOF
#define ACE_HAS_IPV6
#define ACE_USES_IPV4_IPV6_MIGRATION
EOF
%endif

# If rnq support is indicated insert some lines into the config.h file
%if %{?_with_rnq:1}%{!?_with_rnq:0}
cat >> $ACE_ROOT/ace/config.h << EOF
#define ACE_HAS_REACTOR_NOTIFICATION_QUEUE
EOF
%endif

# Include platform include
cat >> $ACE_ROOT/ace/config.h << EOF
#include "ace/config-linux.h"
EOF

# platform_macros.GNU
%if 0%{?suse_version}
cat > $ACE_ROOT/include/makeinclude/platform_macros.GNU <<EOF
CCFLAGS += %optflags
CFLAGS += %optflags
EOF
%endif

cat >> $ACE_ROOT/include/makeinclude/platform_macros.GNU <<EOF
ssl = 1
EOF

%if %{?_with_xt:1}%{!?_with_xt:0}
cat >> $ACE_ROOT/include/makeinclude/platform_macros.GNU <<EOF
xt = 1
ace_xtreactor = 1
x11 = 1
tao_xtresource = 1
EOF
%endif

%if %{?_with_tk:1}%{!?_with_tk:0}
cat >> $ACE_ROOT/include/makeinclude/platform_macros.GNU <<EOF
ace_tkreactor = 1
tao_tkresource = 1
tk = 1
EOF
%endif

%if %{?_with_fl:1}%{!?_with_fl:0}
cat >> $ACE_ROOT/include/makeinclude/platform_macros.GNU <<EOF
fl = 1
tao_flresource = 1
ace_flreactor = 1
EOF
%endif

%if %{?_with_qt:1}%{!?_with_qt:0}
cat >> $ACE_ROOT/include/makeinclude/platform_macros.GNU <<EOF
qt = 1
gl = 1
ace_qtreactor = 1
tao_qtresource = 1
EOF
%endif

%if %{?_with_fox:1}%{!?_with_fox:0}
%if 0%{?have_fox} == 1
cat >> $ACE_ROOT/include/makeinclude/platform_macros.GNU <<EOF
fox = 1
ace_foxreactor = 1
tao_foxresource = 1
%endif
%endif

# 64 bit machines need additional macro.
%ifarch x86_64 ia64 ppc64 s390x
cat >> $ACE_ROOT/include/makeinclude/platform_macros.GNU <<EOF
buildbits = 64
EOF
%endif

%if %{?_with_opt:0}%{!?_with_opt:1}
cat >> $ACE_ROOT/include/makeinclude/platform_macros.GNU <<EOF
optimize = 0
EOF
%else
cat >> $ACE_ROOT/include/makeinclude/platform_macros.GNU <<EOF
optimize = 1
EOF
%endif

cat >> $ACE_ROOT/include/makeinclude/platform_macros.GNU <<EOF
include \$(ACE_ROOT)/include/makeinclude/platform_linux.GNU
EOF

cat > $ACE_ROOT/bin/MakeProjectCreator/config/default.features <<EOF
ssl=1
cidl=0
EOF

%if %{?_with_bzip2:1}%{!?_with_bzip2:0}
cat >> $ACE_ROOT/bin/MakeProjectCreator/config/default.features <<EOF
bzip2 = 1
EOF
%endif

%if %{?_with_zlib:1}%{!?_with_zlib:0}
cat >> $ACE_ROOT/bin/MakeProjectCreator/config/default.features <<EOF
zlib = 1
EOF
%endif


# We don't use default.features to enable ipv6 cause it conflicts w/
# the config.h generated version.  Config.h is superior because it is
# shipped to the end-user machines and also defines
# ACE_USES_IPV4_IPV6_MIGRATION which the default.features technique
# does not seem to set.

%if %{?_with_fox:1}%{!?_with_fox:0}
%if 0%{?have_fox} == 1
cat >> $ACE_ROOT/bin/MakeProjectCreator/config/default.features <<EOF
fox=1
EOF
%endif
%endif

# Need to regenerate all of the GNUMakefiles ...
bin/mwc.pl -type gnuace TAO/TAO_ACE.mwc

MAKECMD="make %{?_smp_mflags}"

# build ACE components
for ace_comp in \
    ace \
    Kokyu \
    ACEXML \
    apps/gperf \
    protocols;
do
    $MAKECMD -C $ACE_ROOT/$ace_comp;
done

# build TAO components
$MAKECMD -C $TAO_ROOT/TAO_IDL
$MAKECMD -C $TAO_ROOT/tao

# Instead of "$MAKECMD -C $TAO_ROOT/orbsvcs" use the list from
# $ACE_ROOT/orbsvcs/GNUmakefile less the performance-tests, tests and
# examples.
for orbsvcs_comp in \
    TAO_Service \
    orbsvcs \
    Trading_Service \
    Time_Service \
    Scheduling_Service \
    Notify_Service \
    Naming_Service \
    Logging_Service \
    LoadBalancer \
    LifeCycle_Service \
    ImplRepo_Service \
    IFR_Service \
    Fault_Notifier \
    Fault_Detector \
    FT_ReplicationManager \
    FTRT_Event_Service \
    Event_Service \
    Dump_Schedule \
    CosEvent_Service \
    Concurrency_Service;
do
    $MAKECMD -C $TAO_ROOT/orbsvcs/$orbsvcs_comp;
done

$MAKECMD -C $TAO_ROOT/utils

%else

autoreconf -fi

mkdir -p objdir && cd objdir

%if %{?_with_opt:0}%{!?_with_opt:1}
export CFLAGS="${CFLAGS:-%optflags} -O0"
export CXXFLAGS="${CXXFLAGS:-%optflags} -O0"
%else
export CFLAGS="${CFLAGS:-%optflags}"
export CXXFLAGS="${CXXFLAGS:-%optflags}"
%endif

../configure --build=%{_build} --host=%{_host} \
        --target=%{_target_platform} \
        --program-prefix=%{?_program_prefix} \
        --prefix=%{_prefix} \
        --exec-prefix=%{_exec_prefix} \
        --bindir=%{_bindir} \
        --sbindir=%{_sbindir} \
        --sysconfdir=%{_sysconfdir} \
        --datadir=%{_datadir} \
        --includedir=%{_includedir} \
        --libdir=%{_libdir} \
        --libexecdir=%{_libexecdir} \
        --localstatedir=%{_localstatedir} \
        --sharedstatedir=%{_sharedstatedir} \
        --mandir=%{_mandir} \
        --infodir=%{_infodir} \
%if %{?_with_ipv6:1}%{!?_with_ipv6:0}
        --enable-ipv4-ipv6 \
        --enable-ipv6     \
%endif
%if %{?_with_rnq:1}%{!?_with_rnq:0}
        --enable-ace-reactor-notification-queue \
%endif
%if %{?_with_qt:1}%{!?_with_qt:0}
        --enable-qt-reactor \
%endif
%if %{?_with_tk:1}%{!?_with_tk:0}
        --enable-tk-reactor \
%endif
%if %{?_with_xt:1}%{!?_with_xt:0}
        --enable-xt-reactor \
%endif
%if %{?_with_fl:1}%{!?_with_fl:0}
        --enable-fl-reactor \
%endif

make %{?jobs:-j%jobs}

%endif
%endif

# ================================================================
# install
# ================================================================

%define ACEVERSO %{ACEVER}
%define TAOVERSO %{TAOVER}
%define CIAOVERSO %{CIAOVER}

%install

export ACE_ROOT=$(pwd)
export TAO_ROOT=$ACE_ROOT/TAO
export CIAO_ROOT=$TAO_ROOT/CIAO
export DANCE_ROOT=$CIAO_ROOT/DAnCE

# ---------------- Runtime Components ----------------

# install shared libraries
install -d %{buildroot}%{_libdir}

# ACE + XML libraries
INSTLIBS=`ls ${ACE_ROOT}/lib/libACE*.so.%{ACEVERSO}`
install $INSTLIBS %{buildroot}%{_libdir}

# ACE-Kokyu libraries
INSTLIBS=`ls ${ACE_ROOT}/lib/libKokyu.so.%{ACEVERSO}`
install $INSTLIBS %{buildroot}%{_libdir}

# TAO libraries
INSTLIBS=`ls ${ACE_ROOT}/lib/libTAO*.so.%{TAOVERSO}`
install $INSTLIBS %{buildroot}%{_libdir}

# Create un-versioned symbolic links for libraries
(cd %{buildroot}%{_libdir} && \
 ls *.so.* | awk 'BEGIN{FS="."}{print "ln -sf " $0 " " $1 "." $2;}' | sh)

# install binaries
install -d %{buildroot}%{_sbindir}

# Rename the service binaries:

install ${ACE_ROOT}/TAO/orbsvcs/Naming_Service/Naming_Service \
    %{buildroot}%{_sbindir}/tao-cosnaming

install ${ACE_ROOT}/TAO/orbsvcs/CosEvent_Service/CosEvent_Service \
    %{buildroot}%{_sbindir}/tao-cosevent

install ${ACE_ROOT}/TAO/orbsvcs/Notify_Service/Notify_Service \
    %{buildroot}%{_sbindir}/tao-cosnotification

install ${ACE_ROOT}/TAO/orbsvcs/Trading_Service/Trading_Service \
    %{buildroot}%{_sbindir}/tao-costrading

install ${ACE_ROOT}/TAO/orbsvcs/Event_Service/Event_Service \
    %{buildroot}%{_sbindir}/tao-rtevent

install ${ACE_ROOT}/TAO/orbsvcs/Concurrency_Service/Concurrency_Service \
    %{buildroot}%{_sbindir}/tao-cosconcurrency

#Create directories
for dir in cache log; do
        mkdir -p %{buildroot}%{_localstatedir}/${dir}/tao
done

# Create data files which will be ghosted.
touch %{buildroot}%{_localstatedir}/cache/tao/tao-cosnaming.dat

# Create data files which will be ghosted.
for logfile in cosnaming cosconcurrency cosevent cosnotification costrading rtevent; do
    touch %{buildroot}%{_localstatedir}/log/tao/tao-${logfile}.log
done

# ---------------- Development Components ----------------

# INSTHDR="cp --preserve=timestamps"
INSTHDR="install -m 0644 -p"

# install headers
install -d %{buildroot}%{_includedir}
( set +x
echo "Building list of headers..."

# Generate raw dependency output
BASEHDR=`find \
    ace \
    ACEXML/common \
    ACEXML/parser/parser \
    Kokyu \
    TAO/tao \
    TAO/orbsvcs/orbsvcs \
    -name '*.h' -not -name 'config-*'`
for j in $BASEHDR; do
        echo $j >> rawhdrs.log
        echo '#include <'$j'>' | \
        g++ -I . \
            -I protocols \
            -I TAO \
            -I TAO/orbsvcs \
            -I TAO/orbsvcs/orbsvcs \
            -x c++ - -MM -MF mmout 2>> rawhdrs.log && cat mmout || true;
done > mmraw.list

# Append IDL headers to the raw list.
find \
    TAO/tao \
    TAO/orbsvcs/orbsvcs \
    -regex '.*\.p?idl$' >> mmraw.list

# Cleanup dependency output:
#   remove '-:' sequences
#   change all sequences of whitespace into \n
#   remove leading './'
#   cannonicalize up to two levels of '/../../'
#   remove duplicates
cat mmraw.list |\
        sed -e 's/^-://g' -e 's/\\//g' | \
        tr -s [:space:] "\n" | \
        sed -e 's#^./##g' | \
        sed -e 's#/[^/]\+/\.\./#/#g' -e 's#/[^/]\+/\.\./#/#g' | \
        sort -u > allhdrs.list

# Add missing headers.
echo ace/QtReactor/QtReactor.h >> allhdrs.list
echo TAO/tao/QtResource/QtResource_Factory.h >> allhdrs.list
echo TAO/tao/QtResource/QtResource_Loader.h >> allhdrs.list
echo TAO/tao/PortableServer/get_arg.h >> allhdrs.list

# Install headers and create header lists
rm -f ace-headers.tmp
rm -f acexml-headers.tmp
rm -f kokyu-headers.tmp
rm -f tao-headers.tmp

for i in `cat allhdrs.list`; do
    case "$i" in
    protocols/ace/*)
        mkdir -p `dirname %{buildroot}%{_includedir}/${i/protocols\/}`
        $INSTHDR $i %{buildroot}%{_includedir}/${i/protocols/}
        echo '%dir %{_includedir}/'`dirname ${i/protocols/}` >> ace-headers.tmp
        echo '%{_includedir}/'${i/protocols/} >> ace-headers.tmp
        ;;
    ace/*)
        mkdir -p `dirname %{buildroot}%{_includedir}/$i`
        $INSTHDR $i %{buildroot}%{_includedir}/$i
        echo '%dir %{_includedir}/'`dirname $i` >> ace-headers.tmp
        echo '%{_includedir}/'$i >> ace-headers.tmp
        ;;
    ACEXML/*)
        mkdir -p `dirname %{buildroot}%{_includedir}/$i`
        $INSTHDR $i %{buildroot}%{_includedir}/$i
        echo '%dir %{_includedir}/'`dirname $i` >> acexml-headers.tmp
        echo '%{_includedir}/'$i >> acexml-headers.tmp
        ;;
    Kokyu/*)
        mkdir -p `dirname %{buildroot}%{_includedir}/$i`
        $INSTHDR $i %{buildroot}%{_includedir}/$i
        echo '%dir %{_includedir}/'`dirname $i` >> kokyu-headers.tmp
        echo '%{_includedir}/'$i >> kokyu-headers.tmp
        ;;
    TAO/tao/*)
        mkdir -p `dirname %{buildroot}%{_includedir}/${i/TAO\/}`
        $INSTHDR $i %{buildroot}%{_includedir}/${i/TAO\/}
        echo '%dir %{_includedir}/'`dirname ${i/TAO\/}` >> tao-headers.tmp
        echo '%{_includedir}/'${i/TAO\/} >> tao-headers.tmp
        ;;
    TAO/orbsvcs/orbsvcs/*)
        mkdir -p `dirname %{buildroot}%{_includedir}/${i/TAO\/orbsvcs\/}`
        $INSTHDR $i %{buildroot}%{_includedir}/${i/TAO\/orbsvcs\/}
        echo '%dir %{_includedir}/'`dirname ${i/TAO\/orbsvcs\/}` >> tao-headers.tmp
        echo '%{_includedir}/'${i/TAO\/orbsvcs\/} >> tao-headers.tmp
        ;;
    *)
        # mkdir -p `dirname %{buildroot}%{_includedir}/$i`
        # $INSTHDR $i %{buildroot}%{_includedir}/$i
        echo $i
        ;;
    esac
done

echo '%defattr(-,root,root,-)' > ace-headers.list
sort -u < ace-headers.tmp >> ace-headers.list
rm -f ace-headers.tmp

echo '%defattr(-,root,root,-)' > acexml-headers.list
sort -u < acexml-headers.tmp >> acexml-headers.list
rm -f acexml-headers.tmp

echo '%defattr(-,root,root,-)' > kokyu-headers.list
sort -u < kokyu-headers.tmp >> kokyu-headers.list
rm -f kokyu-headers.tmp

echo '%defattr(-,root,root,-)' > tao-headers.list
sort -u < tao-headers.tmp >> tao-headers.list
rm -f tao-headers.tmp
)

# install the TAO_IDL compiler
install -d %{buildroot}%{_libdir}

install -d %{buildroot}%{_bindir}
install ${ACE_ROOT}/bin/ace_gperf %{buildroot}%{_bindir}
install ${ACE_ROOT}/bin/tao_idl %{buildroot}%{_bindir}
install ${ACE_ROOT}/bin/tao_imr %{buildroot}%{_bindir}
install ${ACE_ROOT}/bin/tao_ifr %{buildroot}%{_bindir}
install ${ACE_ROOT}/bin/tao_catior %{buildroot}%{_bindir}/tao_catior
install ${ACE_ROOT}/bin/tao_nsadd %{buildroot}%{_bindir}/tao_nsadd
install ${ACE_ROOT}/bin/tao_nsdel %{buildroot}%{_bindir}/tao_nsdel
install ${ACE_ROOT}/bin/tao_nslist %{buildroot}%{_bindir}/tao_nslist

# ================================================================
# Config & Options
# ================================================================

install -d %{buildroot}%{_sysconfdir}
mkdir -p %{buildroot}%{_sysconfdir}/logrotate.d
mkdir -p %{buildroot}%{_sysconfdir}/tao
cp -R ${ACE_ROOT}/rpmbuild/etc/logrotate.d/* %{buildroot}%{_sysconfdir}/logrotate.d/
cp -R ${ACE_ROOT}/rpmbuild/etc/tao/* %{buildroot}%{_sysconfdir}/tao/

%if 0%{?suse_version}
mkdir -p %{buildroot}%{_sysconfdir}/init.d
mkdir -p %{buildroot}%{_localstatedir}/adm
cp -R ${ACE_ROOT}/rpmbuild/ace-tao-init-suse/init.d/* %{buildroot}%{_sysconfdir}/init.d/
cp -R ${ACE_ROOT}/rpmbuild/ace-tao-init-suse/tao/* %{buildroot}%{_sysconfdir}/tao/
%else
mkdir -p %{buildroot}%{_sysconfdir}/rc.d/init.d
cp -R ${ACE_ROOT}/rpmbuild/ace-tao-init-fedora/rc.d/init.d/* %{buildroot}%{_sysconfdir}/rc.d/init.d/
cp -R ${ACE_ROOT}/rpmbuild/ace-tao-init-fedora/tao/* %{buildroot}%{_sysconfdir}/tao/
%endif

%if 0%{?suse_version}
pushd %{buildroot}%{_sysconfdir}/init.d
for f in *; do
        ln -s /etc/init.d/$f %{buildroot}%{_sbindir}/rc${f}
done
popd
%endif

# ================================================================
# Makefiles
# ================================================================

install -d %{buildroot}%{_datadir}
install -d %{buildroot}%{_datadir}/ace
install -d %{buildroot}%{_datadir}/ace/include
install -d %{buildroot}%{_datadir}/ace/include/makeinclude
install -d %{buildroot}%{_datadir}/mpc
install -d %{buildroot}%{_datadir}/tao
install -d %{buildroot}%{_datadir}/tao/orbsvcs
install -d %{buildroot}%{_datadir}/tao/MPC

for mk_macros in \
    all_in_one.GNU \
    component_check.GNU \
    macros.GNU \
    platform_g++_common.GNU \
    platform_linux.GNU \
    platform_linux_common.GNU \
    platform_macros.GNU \
    rules.bin.GNU \
    rules.common.GNU \
    rules.lib.GNU \
    rules.local.GNU \
    rules.nested.GNU \
    rules.nolocal.GNU \
    rules.nonested.GNU \
    wrapper_macros.GNU; do (
        install ${ACE_ROOT}/include/makeinclude/$mk_macros %{buildroot}%{_datadir}/ace/include/makeinclude)
done

install ${TAO_ROOT}/rules.tao.GNU %{buildroot}%{_datadir}/tao

cp -a ${ACE_ROOT}/MPC/* %{buildroot}%{_datadir}/mpc

install -d %{buildroot}%{_datadir}/ace/bin
cp -a ${ACE_ROOT}/bin/DependencyGenerator %{buildroot}%{_datadir}/ace/bin
cp -a ${ACE_ROOT}/bin/MakeProjectCreator %{buildroot}%{_datadir}/ace/bin
install -d %{buildroot}%{_datadir}/ace/bin/PerlACE
cp -a ${ACE_ROOT}/bin/PerlACE/* %{buildroot}%{_datadir}/ace/bin/PerlACE
install ${ACE_ROOT}/bin/mpc.pl %{buildroot}%{_datadir}/ace/bin
install ${ACE_ROOT}/bin/mwc.pl %{buildroot}%{_datadir}/ace/bin
install ${ACE_ROOT}/bin/g++dep %{buildroot}%{_datadir}/ace/bin
install ${ACE_ROOT}/bin/depgen.pl %{buildroot}%{_datadir}/ace/bin
install ${ACE_ROOT}/bin/generate_export_file.pl %{buildroot}%{_datadir}/ace/bin
install ${ACE_ROOT}/bin/add_rel_link.sh %{buildroot}%{_datadir}/ace/bin
install ${ACE_ROOT}/bin/{ACEutils,Uniqueid}.pm %{buildroot}%{_datadir}/ace/bin

ln -sfn %{_includedir}/ace %{buildroot}%{_datadir}/ace
ln -sfn %{_includedir}/tao %{buildroot}%{_datadir}/tao
ln -sfn %{_includedir}/orbsvcs %{buildroot}%{_datadir}/tao/orbsvcs
ln -sfn %{_libdir} %{buildroot}%{_datadir}/ace/lib

cp -a ${TAO_ROOT}/MPC/* %{buildroot}%{_datadir}/tao/MPC

# Set TAO_IDL setting for the user
cat > %{buildroot}%{_datadir}/ace/include/makeinclude/platform_macros.GNU.tmp <<EOF
TAO_IDL = %{_bindir}/tao_idl
TAO_IDL_DEP = %{_bindir}/tao_idl
EOF
cat %{buildroot}%{_datadir}/ace/include/makeinclude/platform_macros.GNU >> %{buildroot}%{_datadir}/ace/include/makeinclude/platform_macros.GNU.tmp
mv %{buildroot}%{_datadir}/ace/include/makeinclude/platform_macros.GNU.tmp %{buildroot}%{_datadir}/ace/include/makeinclude/platform_macros.GNU


install -d %{buildroot}%{_sysconfdir}/profile.d
cat > %{buildroot}%{_sysconfdir}/profile.d/mpc.sh <<EOF
MPC_ROOT=/usr/share/mpc
export MPC_ROOT
EOF
cat > %{buildroot}%{_sysconfdir}/profile.d/ace-devel.sh <<EOF
ACE_ROOT=/usr/share/ace
export ACE_ROOT
EOF
cat > %{buildroot}%{_sysconfdir}/profile.d/tao-devel.sh <<EOF
TAO_ROOT=/usr/share/tao
export TAO_ROOT
EOF

# convenience symlinks
ln -sfn %{_datadir}/ace/bin/mpc.pl %{buildroot}%{_bindir}/mpc.pl
ln -sfn %{_datadir}/ace/bin/mwc.pl %{buildroot}%{_bindir}/mwc.pl

# ================================================================
# Manuals
# ================================================================
install -d %{buildroot}%{_mandir}
install -d %{buildroot}%{_mandir}/man1
install ${TAO_ROOT}/TAO_IDL/tao_idl.1 %{buildroot}%{_mandir}/man1
install ${ACE_ROOT}/apps/gperf/ace_gperf.1 %{buildroot}%{_mandir}/man1
install -d  %{buildroot}%{_infodir}
install ${ACE_ROOT}/apps/gperf/ace_gperf.info %{buildroot}%{_infodir}

# ================================================================
# Create lists of symlinked so's.  We need two lists because we need
# the unversioned symlinks in the runtime package for so's that can
# be loaded in the svc.conf.
# ================================================================

# Make a list of all shared objects.
(cd %{buildroot}/%{_libdir} && ls *.so | \
        awk '{ print "%{_libdir}/"$1; }' | \
        sort) > all-so.list

# Make a list of likely svc.conf targets.
(cd %{buildroot}/%{_libdir} && ls *.so | \
    nm --print-file-name *.so | \
    grep _make_ | \
    awk 'BEGIN { FS=":"} /^[^:]+:/ { print "%{_libdir}/"$1; }' | \
    sort -u) > rough-svc-so.list

# Remove false positives (IMPORTANT keep this list sorted!)
cat > falsepos-svc-so.list <<EOF
%{_libdir}/libACE.so
%{_libdir}/libTAO.so
EOF
comm -2 -3 rough-svc-so.list falsepos-svc-so.list > svc-so.list

# Find the list of non-sv.conf target files.
comm -2 -3 all-so.list svc-so.list > nonsvc-so.list

# Generate file lists.
grep libACE svc-so.list > ace-svc-so.list
grep libACE nonsvc-so.list > ace-nonsvc-so.list
grep libTAO svc-so.list > tao-svc-so.list
grep libTAO nonsvc-so.list > tao-nonsvc-so.list

# Concatenate file lists as neccessary
cat tao-headers.list tao-nonsvc-so.list > tao-devel-files.list
cat ace-headers.list ace-nonsvc-so.list > ace-devel-files.list

# ================================================================
# clean
# ================================================================

%clean
rm -rf %{buildroot}

# ================================================================
# pre install
# ================================================================

# ---------------- tao-cosnaming ----------------

%pre -n tao-cosnaming

getent group tao >/dev/null || /usr/sbin/groupadd -r tao
getent passwd tao >/dev/null || \
/usr/sbin/useradd -r -g tao -d %{_sysconfdir}/tao -s /sbin/nologin \
    -c "TAO Services" tao
exit 0

# ---------------- tao-cosevent ----------------

%pre -n tao-cosevent

getent group tao >/dev/null || /usr/sbin/groupadd -r tao
getent passwd tao >/dev/null || \
/usr/sbin/useradd -r -g tao -d %{_sysconfdir}/tao -s /sbin/nologin \
    -c "TAO Services" tao
exit 0

# ---------------- tao-cosnotification ----------------

%pre -n tao-cosnotification

getent group tao >/dev/null || /usr/sbin/groupadd -r tao
getent passwd tao >/dev/null || \
/usr/sbin/useradd -r -g tao -d %{_sysconfdir}/tao -s /sbin/nologin \
    -c "TAO Services" tao
exit 0

# ---------------- tao-costrading ----------------

%pre -n tao-costrading

getent group tao >/dev/null || /usr/sbin/groupadd -r tao
getent passwd tao >/dev/null || \
/usr/sbin/useradd -r -g tao -d %{_sysconfdir}/tao -s /sbin/nologin \
    -c "TAO Services" tao
exit 0

# ---------------- tao-rtevent ----------------

%pre -n tao-rtevent

getent group tao >/dev/null || /usr/sbin/groupadd -r tao
getent passwd tao >/dev/null || \
/usr/sbin/useradd -r -g tao -d %{_sysconfdir}/tao -s /sbin/nologin \
    -c "TAO Services" tao
exit 0

# ---------------- tao-cosconcurrency ----------------

%pre -n tao-cosconcurrency

getent group tao >/dev/null || /usr/sbin/groupadd -r tao
getent passwd tao >/dev/null || \
/usr/sbin/useradd -r -g tao -d %{_sysconfdir}/tao -s /sbin/nologin \
    -c "TAO Services" tao
exit 0

# ================================================================
# post install
# ================================================================

# ---------------- ace ----------------

%post -n ace
/sbin/ldconfig

# ---------------- ace-devel ----------------

%post -n ace-devel
/sbin/ldconfig

# ---------------- ace-xml ----------------

%post -n ace-xml
/sbin/ldconfig

# ---------------- ace-gperf ----------------

%post -n ace-gperf

%if 0%{?suse_version}
%install_info --info-dir=%_infodir %_infodir/ace_gperf.info%{_extension}
%else
/sbin/install-info %{_infodir}/ace_gperf.info%{_extension} %{_infodir}/dir
%endif

# ---------------- ace-kokyu ----------------

%post -n ace-kokyu
/sbin/ldconfig

# ---------------- ace-foxreactor ----------------

%if %{?_with_fox:1}%{!?_with_fox:0}
%if 0%{!?suse_version} || 0%{?suse_version} == 1020
%post -n ace-foxreactor
/sbin/ldconfig
%endif
%endif

# ---------------- ace-flreactor ----------------

%if %{?_with_fl:1}%{!?_with_fl:0}
%post -n ace-flreactor
/sbin/ldconfig
%endif

# ---------------- ace-qtreactor ----------------

%if %{?_with_qt:1}%{!?_with_qt:0}
%post -n ace-qtreactor
/sbin/ldconfig
%endif

# ---------------- ace-tkreactor ----------------

%if %{?_with_tk:1}%{!?_with_tk:0}
%post -n ace-tkreactor
/sbin/ldconfig
%endif

# ---------------- ace-xtreactor ----------------

%if %{?_with_xt:1}%{!?_with_xt:0}
%post -n ace-xtreactor
/sbin/ldconfig
%endif

# ---------------- tao ----------------

%post -n tao
/sbin/ldconfig

# ---------------- tao-devel ----------------

%post -n tao-devel
/sbin/ldconfig

# ---------------- tao-utils ----------------

%post -n tao-utils
/sbin/ldconfig

# ---------------- tao-cosnaming ----------------

%post -n tao-cosnaming
%if 0%{?suse_version}
%{fillup_and_insserv tao-cosnaming}
%else
/sbin/chkconfig --add tao-cosnaming
%endif

# ---------------- tao-cosevent ----------------

%post -n tao-cosevent

%if 0%{?suse_version}
%{fillup_and_insserv tao-cosevent}
%else
/sbin/chkconfig --add tao-cosevent
%endif

# ---------------- tao-cosnotification ----------------

%post -n tao-cosnotification

%if 0%{?suse_version}
%{fillup_and_insserv tao-cosnotification}
%else
/sbin/chkconfig --add tao-cosnotification
%endif

# ---------------- tao-costrading ----------------

%post -n tao-costrading

%if 0%{?suse_version}
%{fillup_and_insserv tao-costrading}
%else
/sbin/chkconfig --add tao-costrading
%endif

# ---------------- tao-rtevent ----------------

%post -n tao-rtevent

%if 0%{?suse_version}
%{fillup_and_insserv tao-rtevent}
%else
/sbin/chkconfig --add tao-rtevent
%endif

# ---------------- tao-cosconcurrency ----------------

%post -n tao-cosconcurrency

%if 0%{?suse_version}
%{fillup_and_insserv tao-cosconcurrency}
%else
/sbin/chkconfig --add tao-cosconcurrency
%endif

# ---------------- tao-flresource ----------------

%if %{?_with_fl:1}%{!?_with_fl:0}
%post -n tao-flresource
/sbin/ldconfig
%endif

# ---------------- tao-qtresource ----------------

%if %{?_with_qt:1}%{!?_with_qt:0}
%post -n tao-qtresource
/sbin/ldconfig
%endif

# ---------------- tao-tkresource ----------------

%if %{?_with_tk:1}%{!?_with_tk:0}
%post -n tao-tkresource
/sbin/ldconfig
%endif

# ---------------- tao-xtresource ----------------

%if %{?_with_xt:1}%{!?_with_xt:0}
%post -n tao-xtresource
/sbin/ldconfig
%endif

# ================================================================
# pre uninstall
# ================================================================

# ---------------- ace-gperf ----------------

%preun -n ace-gperf

if [ $1 = 0 ]; then
    /sbin/install-info --delete %{_infodir}/ace_gperf.info%{_extension} %{_infodir}/dir
fi

# ---------------- tao-cosnaming ----------------

%preun -n tao-cosnaming
%if 0%{?suse_version}
%stop_on_removal tao-cosnaming
%else
if [ $1 = 0 ]; then
    /sbin/service tao-cosnaming stop > /dev/null 2>&1
    /sbin/chkconfig --del tao-cosnaming
fi
%endif

# ---------------- tao-cosevent ----------------

%preun -n tao-cosevent

%if 0%{?suse_version}
%stop_on_removal tao-cosevent
%else
if [ $1 = 0 ]; then
    /sbin/service tao-cosevent stop > /dev/null 2>&1
    /sbin/chkconfig --del tao-cosevent
fi
%endif

# ---------------- tao-cosnotification ----------------

%preun -n tao-cosnotification

%if 0%{?suse_version}
%stop_on_removal tao-cosnotification
%else
if [ $1 = 0 ]; then
    /sbin/service tao-cosnotification stop > /dev/null 2>&1
    /sbin/chkconfig --del tao-cosnotification
fi
%endif

# ---------------- tao-costrading ----------------

%preun -n tao-costrading

%if 0%{?suse_version}
%stop_on_removal tao-costrading
%else
if [ $1 = 0 ]; then
    /sbin/service tao-costrading stop > /dev/null 2>&1
    /sbin/chkconfig --del tao-costrading
fi
%endif

# ---------------- tao-rtevent ----------------

%preun -n tao-rtevent

%if 0%{?suse_version}
%stop_on_removal tao-rtevent
%else
if [ $1 = 0 ]; then
    /sbin/service tao-rtevent stop > /dev/null 2>&1
    /sbin/chkconfig --del tao-rtevent
fi
%endif

# ---------------- tao-cosconcurrency ----------------

%preun -n tao-cosconcurrency

%if 0%{?suse_version}
%stop_on_removal tao-cosconcurrency
%else
if [ $1 = 0 ]; then
    /sbin/service tao-cosconcurrency stop > /dev/null 2>&1
    /sbin/chkconfig --del tao-cosconcurrency
fi
%endif

# ================================================================
# post uninstall
# ================================================================

# ---------------- ace ----------------

%postun -n ace
/sbin/ldconfig

# ---------------- ace-xml ----------------

%postun -n ace-xml
/sbin/ldconfig

# ---------------- ace-kokyu ----------------

%postun -n ace-kokyu
/sbin/ldconfig

# ---------------- ace-foxreactor ----------------

%if %{?_with_fox:1}%{!?_with_fox:0}
%if 0%{?have_fox} == 1
%postun -n ace-foxreactor
/sbin/ldconfig
%endif
%endif

# ---------------- ace-flreactor ----------------

%if %{?_with_fl:1}%{!?_with_fl:0}
%postun -n ace-flreactor
/sbin/ldconfig
%endif

# ---------------- ace-qtreactor ----------------

%if %{?_with_qt:1}%{!?_with_qt:0}
%postun -n ace-qtreactor
/sbin/ldconfig
%endif

# ---------------- ace-tkreactor ----------------

%if %{?_with_tk:1}%{!?_with_tk:0}
%postun -n ace-tkreactor
/sbin/ldconfig
%endif

# ---------------- ace-xtreactor ----------------

%if %{?_with_xt:1}%{!?_with_xt:0}
%postun -n ace-xtreactor
/sbin/ldconfig
%endif

# ---------------- tao ----------------

%postun -n tao
/sbin/ldconfig

# ---------------- tao-devel ----------------

%postun -n tao-devel
/sbin/ldconfig

# ---------------- tao-utils ----------------

%postun -n tao-utils
/sbin/ldconfig

# ---------------- tao-cosnaming ----------------

%postun -n tao-cosnaming

%if 0%{?suse_version}
%restart_on_update tao-cosnaming
%insserv_cleanup
%else
if [ "$1" -ge "1" ]; then
    /sbin/service tao-cosnaming %{cond_restart} > /dev/null 2>&1
fi
%endif

# ---------------- tao-cosevent ----------------

%postun -n tao-cosevent

%if 0%{?suse_version}
%restart_on_update tao-cosevent
%insserv_cleanup
%else
if [ "$1" -ge "1" ]; then
    /sbin/service tao-cosevent %{cond_restart} > /dev/null 2>&1
fi
%endif

# ---------------- tao-cosnotification ----------------

%postun -n tao-cosnotification

%if 0%{?suse_version}
%restart_on_update tao-cosnotification
%insserv_cleanup
%else
if [ "$1" -ge "1" ]; then
    /sbin/service tao-cosnotification %{cond_restart} > /dev/null 2>&1
fi
%endif

# ---------------- tao-costrading ----------------

%postun -n tao-costrading

%if 0%{?suse_version}
%restart_on_update tao-costrading
%insserv_cleanup
%else
if [ "$1" -ge "1" ]; then
    /sbin/service tao-costrading %{cond_restart} > /dev/null 2>&1
fi
%endif

# ---------------- tao-rtevent ----------------

%postun -n tao-rtevent

%if 0%{?suse_version}
%restart_on_update tao-rtevent
%insserv_cleanup
%else
if [ "$1" -ge "1" ]; then
    /sbin/service tao-rtevent %{cond_restart} > /dev/null 2>&1
fi
%endif

# ---------------- tao-cosconcurrency ----------------

%postun -n tao-cosconcurrency

%if 0%{?suse_version}
%restart_on_update tao-cosconcurrency
%insserv_cleanup
%else
if [ "$1" -ge "1" ]; then
    /sbin/service tao-cosconcurrency %{cond_restart} > /dev/null 2>&1
fi
%endif

# ---------------- tao-flresource ----------------

%if %{?_with_fl:1}%{!?_with_fl:0}
%postun -n tao-flresource
/sbin/ldconfig
%endif

# ---------------- tao-qtresource ----------------

%if %{?_with_qt:1}%{!?_with_qt:0}
%postun -n tao-qtresource
/sbin/ldconfig
%endif

# ---------------- tao-tkresource ----------------

%if %{?_with_tk:1}%{!?_with_tk:0}
%postun -n tao-tkresource
/sbin/ldconfig
%endif

# ---------------- tao-xtresource ----------------

%if %{?_with_xt:1}%{!?_with_xt:0}
%postun -n tao-xtresource
/sbin/ldconfig
%endif

# ================================================================
# files
# ================================================================

# ---------------- ace ----------------

%files -n ace
%defattr(-,root,root,-)
%{_libdir}/libACE.so.%{ACEVERSO}
%{_libdir}/libACE_ETCL_Parser.so.%{ACEVERSO}
%{_libdir}/libACE_ETCL.so.%{ACEVERSO}
%{_libdir}/libACE_HTBP.so.%{ACEVERSO}
%{_libdir}/libACE_Monitor_Control.so.%{ACEVERSO}
%{_libdir}/libACE_RMCast.so.%{ACEVERSO}
%{_libdir}/libACE_TMCast.so.%{ACEVERSO}
%{_libdir}/libACE_SSL.so.%{ACEVERSO}
%{_libdir}/libACE_INet.so.%{ACEVERSO}
%{_libdir}/libACE_INet_SSL.so.%{ACEVERSO}

%doc ACE-INSTALL.html
%doc AUTHORS
%doc COPYING
%doc FAQ
%doc PROBLEM-REPORT-FORM
%doc README
%doc THANKS
%doc VERSION

# ---------------- ace-devel ----------------

%files -n ace-devel -f ace-devel-files.list
%defattr(-,root,root,-)
%{_libdir}/libACE.so
%{_libdir}/libACE_ETCL_Parser.so
%{_libdir}/libACE_ETCL.so
%{_libdir}/libACE_HTBP.so
%{_libdir}/libACE_Monitor_Control.so
%{_libdir}/libACE_RMCast.so
%{_libdir}/libACE_TMCast.so
%{_libdir}/libACE_SSL.so
%{_libdir}/libACE_INet.so
%{_libdir}/libACE_INet_SSL.so
%dir %{_datadir}/ace
%{_datadir}/ace/include
%{_datadir}/ace/include/makeinclude
%{_datadir}/ace/bin
%{_datadir}/ace/ace
%{_datadir}/ace/lib
%config %{_sysconfdir}/profile.d/ace-devel.sh

%if %{?_with_fox:1}%{!?_with_fox:0}
%exclude %{_includedir}/ace/FoxReactor/FoxReactor.h
%exclude %{_includedir}/ace/FoxReactor/ACE_FoxReactor_export.h
%endif
%if %{?_with_fl:1}%{!?_with_fl:0}
%exclude %{_includedir}/ace/FlReactor/FlReactor.h
%exclude %{_includedir}/ace/FlReactor/ACE_FlReactor_export.h
%endif
%if %{?_with_qt:1}%{!?_with_qt:0}
%exclude %{_includedir}/ace/QtReactor/QtReactor.h
%exclude %{_includedir}/ace/QtReactor/ACE_QtReactor_export.h
%endif
%if %{?_with_tk:1}%{!?_with_tk:0}
%exclude %{_includedir}/ace/TkReactor/TkReactor.h
%exclude %{_includedir}/ace/TkReactor/ACE_TkReactor_export.h
%endif
%if %{?_with_xt:1}%{!?_with_xt:0}
%exclude %{_includedir}/ace/XtReactor/XtReactor.h
%exclude %{_includedir}/ace/XtReactor/ACE_XtReactor_export.h
%endif

%doc AUTHORS
%doc COPYING
%doc PROBLEM-REPORT-FORM
%doc README
%doc VERSION

# ---------------- ace-xml ----------------

%files -n ace-xml
%defattr(-,root,root,-)
%{_libdir}/libACEXML*.so.%{ACEVERSO}

%doc AUTHORS
%doc COPYING
%doc PROBLEM-REPORT-FORM
%doc README
%doc VERSION

# ---------------- ace-gperf ----------------

%files -n ace-gperf
%defattr(-,root,root,-)
%{_bindir}/ace_gperf
%attr(0644,root,root) %{_mandir}/man1/ace_gperf.1%{_extension}
%attr(0644,root,root) %{_infodir}/ace_gperf.info%{_extension}

%doc AUTHORS
%doc COPYING
%doc PROBLEM-REPORT-FORM
%doc README
%doc VERSION

# ---------------- ace-xml-devel ----------------

%files -n ace-xml-devel -f acexml-headers.list
%defattr(-,root,root,-)
%{_libdir}/libACEXML*.so

# These get missed by the automatic list generator because they
# contain no immediate files.
%dir %{_includedir}/ACEXML/parser
%dir %{_includedir}/ACEXML

%doc AUTHORS
%doc COPYING
%doc PROBLEM-REPORT-FORM
%doc README
%doc VERSION

# ---------------- ace-kokyu ----------------

%files -n ace-kokyu
%defattr(-,root,root,-)
%{_libdir}/libKokyu.so.%{ACEVERSO}

%doc AUTHORS
%doc COPYING
%doc PROBLEM-REPORT-FORM
%doc README
%doc VERSION

# ---------------- ace-kokyu-devel ----------------

%files -n ace-kokyu-devel -f kokyu-headers.list
%defattr(-,root,root,-)
%{_libdir}/libKokyu.so

%doc AUTHORS
%doc COPYING
%doc PROBLEM-REPORT-FORM
%doc README
%doc VERSION

# ---------------- ace-foxreactor ----------------

%if 0%{?have_fox} == 1
%if %{?_with_fox:1}%{!?_with_fox:0}
%files -n ace-foxreactor
%defattr(-,root,root,-)
%{_libdir}/libACE_FoxReactor.so.%{ACEVERSO}

%doc AUTHORS
%doc COPYING
%doc PROBLEM-REPORT-FORM
%doc README
%doc VERSION

%endif
%endif
# ---------------- ace-flreactor ----------------

%if %{?_with_fl:1}%{!?_with_fl:0}

%files -n ace-flreactor
%defattr(-,root,root,-)
%{_libdir}/libACE_FlReactor.so.%{ACEVERSO}

%doc AUTHORS
%doc COPYING
%doc PROBLEM-REPORT-FORM
%doc README
%doc VERSION

%endif

# ---------------- ace-flreactor-devel ----------------

%if %{?_with_fl:1}%{!?_with_fl:0}

%files -n ace-flreactor-devel
%defattr(-,root,root,-)
%dir %{_includedir}/ace/FlReactor
%{_libdir}/libACE_FlReactor.so
%{_includedir}/ace/FlReactor/FlReactor.h
%{_includedir}/ace/FlReactor/ACE_FlReactor_export.h

%doc AUTHORS
%doc COPYING
%doc PROBLEM-REPORT-FORM
%doc README
%doc VERSION

%endif

# ---------------- ace-qtreactor ----------------

%if %{?_with_qt:1}%{!?_with_qt:0}

%files -n ace-qtreactor
%defattr(-,root,root,-)
%{_libdir}/libACE_QtReactor.so.%{ACEVERSO}

%doc AUTHORS
%doc COPYING
%doc PROBLEM-REPORT-FORM
%doc README
%doc VERSION

%endif

# ---------------- ace-qtreactor-devel ----------------

%if %{?_with_qt:1}%{!?_with_qt:0}

%files -n ace-qtreactor-devel
%defattr(-,root,root,-)
%{_libdir}/libACE_QtReactor.so
%dir %{_includedir}/ace/QtReactor
%{_includedir}/ace/QtReactor/QtReactor.h
%{_includedir}/ace/QtReactor/ACE_QtReactor_export.h

%doc AUTHORS
%doc COPYING
%doc PROBLEM-REPORT-FORM
%doc README
%doc VERSION

%endif

# ---------------- ace-tkreactor ----------------

%if %{?_with_tk:1}%{!?_with_tk:0}

%files -n ace-tkreactor
%defattr(-,root,root,-)
%{_libdir}/libACE_TkReactor.so.%{ACEVERSO}

%doc AUTHORS
%doc COPYING
%doc PROBLEM-REPORT-FORM
%doc README
%doc VERSION

%endif

# ---------------- ace-tkreactor-devel ----------------

%if %{?_with_tk:1}%{!?_with_tk:0}

%files -n ace-tkreactor-devel
%defattr(-,root,root,-)
%{_libdir}/libACE_TkReactor.so
%dir %{_includedir}/ace/TkReactor
%{_includedir}/ace/TkReactor/TkReactor.h
%{_includedir}/ace/TkReactor/ACE_TkReactor_export.h

%doc AUTHORS
%doc COPYING
%doc PROBLEM-REPORT-FORM
%doc README
%doc VERSION

%endif

# ---------------- ace-xtreactor ----------------

%if %{?_with_xt:1}%{!?_with_xt:0}

%files -n ace-xtreactor
%defattr(-,root,root,-)
%{_libdir}/libACE_XtReactor.so.%{ACEVERSO}

%doc AUTHORS
%doc COPYING
%doc PROBLEM-REPORT-FORM
%doc README
%doc VERSION

%endif

# ---------------- ace-xtreactor-devel ----------------

%if %{?_with_xt:1}%{!?_with_xt:0}

%files -n ace-xtreactor-devel
%defattr(-,root,root,-)
%{_libdir}/libACE_XtReactor.so
%dir %{_includedir}/ace/XtReactor
%{_includedir}/ace/XtReactor/XtReactor.h
%{_includedir}/ace/XtReactor/ACE_XtReactor_export.h

%doc AUTHORS
%doc COPYING
%doc PROBLEM-REPORT-FORM
%doc README
%doc VERSION

%endif

# ---------------- mpc ----------------

%files -n mpc
%defattr(-,root,root,-)
%{_datadir}/mpc
%config %{_sysconfdir}/profile.d/mpc.sh
%{_bindir}/mpc.pl
%{_bindir}/mwc.pl

# ---------------- tao ----------------

# NOTE - Some of the TAO service modules need to be found by dlopen at
# runtime.  Currently this means these specific .so files need to be
# shipped in the runtime package instead of the devel package.

%files -n tao -f tao-svc-so.list
%defattr(-,root,root,-)
%{_datadir}/tao

%{_libdir}/libTAO*.so.%{TAOVERSO}

%if %{?_with_fl:1}%{!?_with_fl:0}
%exclude %{_libdir}/libTAO_FlResource.so*
%endif
%if %{?_with_qt:1}%{!?_with_qt:0}
%exclude %{_libdir}/libTAO_QtResource.so*
%endif
%if %{?_with_tk:1}%{!?_with_tk:0}
%exclude %{_libdir}/libTAO_TkResource.so*
%endif
%if %{?_with_xt:1}%{!?_with_xt:0}
%exclude %{_libdir}/libTAO_XtResource.so*
%endif

%doc TAO/COPYING
%doc TAO/PROBLEM-REPORT-FORM
%doc TAO/TAO-INSTALL.html
%doc TAO/VERSION
%doc TAO/README

# ---------------- tao-devel ----------------

# NOTE - Some of the TAO service modules need to be found by dlopen at
# runtime.  Currently this means these specific .so files need to be
# shipped in the runtime package instead of the devel package.

%files -n tao-devel -f tao-devel-files.list
%defattr(-,root,root,-)
%config %{_sysconfdir}/profile.d/tao-devel.sh

%{_bindir}/tao_imr
%{_bindir}/tao_ifr
%{_datadir}/tao/MPC
%{_bindir}/tao_idl
%attr(0644,root,root) %doc %{_mandir}/man1/tao_idl.1%{_extension}
%{_datadir}/tao/tao
%{_datadir}/tao/orbsvcs

# These get missed by the automatic list generator because they
# contain no immediate files.
%dir %{_includedir}/orbsvcs/FtRtEvent

%if %{?_with_fl:1}%{!?_with_fl:0}
%exclude %{_includedir}/tao/FlResource/FlResource_Factory.h
%exclude %{_includedir}/tao/FlResource/FlResource_Loader.h
%exclude %{_includedir}/tao/FlResource/TAO_FlResource_Export.h
%exclude %{_libdir}/libTAO_FlResource.so
%endif
%if %{?_with_qt:1}%{!?_with_qt:0}
%exclude %{_includedir}/tao/QtResource/QtResource_Factory.h
%exclude %{_includedir}/tao/QtResource/QtResource_Loader.h
%exclude %{_includedir}/tao/QtResource/TAO_QtResource_Export.h
%exclude %{_libdir}/libTAO_QtResource.so
%endif
%if %{?_with_tk:1}%{!?_with_tk:0}
%exclude %{_includedir}/tao/TkResource/TkResource_Factory.h
%exclude %{_includedir}/tao/TkResource/TkResource_Loader.h
%exclude %{_includedir}/tao/TkResource/TAO_TkResource_Export.h
%exclude %{_libdir}/libTAO_TkResource.so
%endif
%if %{?_with_xt:1}%{!?_with_xt:0}
%exclude %{_includedir}/tao/XtResource/XtResource_Factory.h
%exclude %{_includedir}/tao/XtResource/XtResource_Loader.h
%exclude %{_includedir}/tao/XtResource/TAO_XtResource_Export.h
%exclude %{_libdir}/libTAO_XtResource.so
%endif

%doc TAO/COPYING
%doc TAO/PROBLEM-REPORT-FORM
%doc TAO/VERSION
%doc TAO/README

# ---------------- tao-utils ----------------

%files -n tao-utils
%defattr(-,root,root,-)

%{_bindir}/tao_catior
%{_bindir}/tao_nsadd
%{_bindir}/tao_nsdel
%{_bindir}/tao_nslist

%doc TAO/COPYING
%doc TAO/PROBLEM-REPORT-FORM
%doc TAO/VERSION
%doc TAO/README
%doc TAO/utils/catior/README.catior
%doc TAO/utils/nslist/README.nslist

# ---------------- tao-cosnaming ----------------

%files -n tao-cosnaming
%defattr(-,root,root,-)

%dir %{_sysconfdir}/tao

%{_sbindir}/tao-cosnaming

%if 0%{?suse_version}
%{_sysconfdir}/init.d/tao-cosnaming
%{_sbindir}/rctao-cosnaming
%{_sysconfdir}/tao/tao-cosnaming
%else
%{_sysconfdir}/rc.d/init.d/tao-cosnaming
%config(noreplace) %{_sysconfdir}/tao/tao-cosnaming.opt
%endif

%config(noreplace) %{_sysconfdir}/tao/tao-cosnaming.conf
%config(noreplace) %{_sysconfdir}/logrotate.d/tao-cosnaming
%attr(-,tao,tao) %dir %{_localstatedir}/cache/tao
%attr(0644,tao,tao) %ghost %{_localstatedir}/cache/tao/tao-cosnaming.dat
%attr(-,tao,tao) %dir %{_localstatedir}/log/tao
%attr(0644,tao,tao) %ghost %{_localstatedir}/log/tao/tao-cosnaming.log

%doc TAO/COPYING
%doc TAO/PROBLEM-REPORT-FORM
%doc TAO/VERSION
%doc TAO/README

# ---------------- tao-cosevent ----------------

%files -n tao-cosevent
%defattr(-,root,root,-)

%dir %{_sysconfdir}/tao
%{_sbindir}/tao-cosevent

%if 0%{?suse_version}
%{_sysconfdir}/init.d/tao-cosevent
%{_sbindir}/rctao-cosevent
%{_sysconfdir}/tao/tao-cosevent
%else
%{_sysconfdir}/rc.d/init.d/tao-cosevent
%config(noreplace) %{_sysconfdir}/tao/tao-cosevent.opt
%endif

%config(noreplace) %{_sysconfdir}/tao/tao-cosevent.conf
%config(noreplace) %{_sysconfdir}/logrotate.d/tao-cosevent
%attr(-,tao,tao) %dir %{_localstatedir}/log/tao
%attr(0644,tao,tao) %ghost %{_localstatedir}/log/tao/tao-cosevent.log

%doc TAO/COPYING
%doc TAO/PROBLEM-REPORT-FORM
%doc TAO/VERSION
%doc TAO/README

# ---------------- tao-cosnotification ----------------

%files -n tao-cosnotification
%defattr(-,root,root,-)

%{_sbindir}/tao-cosnotification
%dir %{_sysconfdir}/tao

%if 0%{?suse_version}
%{_sysconfdir}/init.d/tao-cosnotification
%{_sbindir}/rctao-cosnotification
%{_sysconfdir}/tao/tao-cosnotification
%else
%{_sysconfdir}/rc.d/init.d/tao-cosnotification
%config(noreplace) %{_sysconfdir}/tao/tao-cosnotification.opt
%endif

%config(noreplace) %{_sysconfdir}/tao/tao-cosnotification.conf
%config(noreplace) %{_sysconfdir}/logrotate.d/tao-cosnotification
%attr(-,tao,tao) %dir %{_localstatedir}/log/tao
%attr(0644,tao,tao) %ghost %{_localstatedir}/log/tao/tao-cosnotification.log

%doc TAO/COPYING
%doc TAO/PROBLEM-REPORT-FORM
%doc TAO/VERSION
%doc TAO/README

# ---------------- tao-costrading ----------------

%files -n tao-costrading
%defattr(-,root,root,-)

%dir %{_sysconfdir}/tao

%{_sbindir}/tao-costrading

%if 0%{?suse_version}
%{_sysconfdir}/init.d/tao-costrading
%{_sbindir}/rctao-costrading
%{_sysconfdir}/tao/tao-costrading
%else
%{_sysconfdir}/rc.d/init.d/tao-costrading
%config(noreplace) %{_sysconfdir}/tao/tao-costrading.opt
%endif

%config(noreplace) %{_sysconfdir}/tao/tao-costrading.conf
%config(noreplace) %{_sysconfdir}/logrotate.d/tao-costrading
%attr(-,tao,tao) %dir %{_localstatedir}/log/tao
%attr(0644,tao,tao) %ghost %{_localstatedir}/log/tao/tao-costrading.log

%doc TAO/COPYING
%doc TAO/PROBLEM-REPORT-FORM
%doc TAO/VERSION
%doc TAO/README

# ---------------- tao-rtevent ----------------

%files -n tao-rtevent
%defattr(-,root,root,-)

%dir %{_sysconfdir}/tao
%{_sbindir}/tao-rtevent

%if 0%{?suse_version}
%{_sysconfdir}/init.d/tao-rtevent
%{_sbindir}/rctao-rtevent
%{_sysconfdir}/tao/tao-rtevent
%else
%{_sysconfdir}/rc.d/init.d/tao-rtevent
%config(noreplace) %{_sysconfdir}/tao/tao-rtevent.opt
%endif

%config(noreplace) %{_sysconfdir}/tao/tao-rtevent.conf
%config(noreplace) %{_sysconfdir}/logrotate.d/tao-rtevent
%attr(-,tao,tao) %dir %{_localstatedir}/log/tao
%attr(0644,tao,tao) %ghost %{_localstatedir}/log/tao/tao-rtevent.log

%doc TAO/COPYING
%doc TAO/PROBLEM-REPORT-FORM
%doc TAO/VERSION
%doc TAO/README

# ---------------- tao-cosconcurrency ----------------

%files -n tao-cosconcurrency
%defattr(-,root,root,-)

%dir %{_sysconfdir}/tao
%{_sbindir}/tao-cosconcurrency

%if 0%{?suse_version}
%{_sysconfdir}/init.d/tao-cosconcurrency
%{_sbindir}/rctao-cosconcurrency
%{_sysconfdir}/tao/tao-cosconcurrency
%else
%{_sysconfdir}/rc.d/init.d/tao-cosconcurrency
%config(noreplace) %{_sysconfdir}/tao/tao-cosconcurrency.opt
%endif

%config(noreplace) %{_sysconfdir}/tao/tao-cosconcurrency.conf
%config(noreplace) %{_sysconfdir}/logrotate.d/tao-cosconcurrency
%attr(-,tao,tao) %dir %{_localstatedir}/log/tao
%attr(0644,tao,tao) %ghost %{_localstatedir}/log/tao/tao-cosconcurrency.log

%doc TAO/COPYING
%doc TAO/PROBLEM-REPORT-FORM
%doc TAO/VERSION
%doc TAO/README

# ---------------- tao-flresource ----------------

%if %{?_with_fl:1}%{!?_with_fl:0}

%files -n tao-flresource
%defattr(-,root,root,-)
%{_libdir}/libTAO_FlResource.so.%{TAOVERSO}

%doc TAO/COPYING
%doc TAO/PROBLEM-REPORT-FORM
%doc TAO/VERSION
%doc TAO/README

%endif

# ---------------- tao-flresource-devel ----------------

%if %{?_with_fl:1}%{!?_with_fl:0}

%files -n tao-flresource-devel
%defattr(-,root,root,-)
%{_libdir}/libTAO_FlResource.so
%dir %{_includedir}/tao
%{_includedir}/tao/FlResource/FlResource_Factory.h
%{_includedir}/tao/FlResource/FlResource_Loader.h
%{_includedir}/tao/FlResource/TAO_FlResource_Export.h

%doc TAO/COPYING
%doc TAO/PROBLEM-REPORT-FORM
%doc TAO/VERSION
%doc TAO/README

%endif

# ---------------- tao-qtresource ----------------

%if %{?_with_qt:1}%{!?_with_qt:0}

%files -n tao-qtresource
%defattr(-,root,root,-)
%{_libdir}/libTAO_QtResource.so.%{TAOVERSO}

%doc TAO/COPYING
%doc TAO/PROBLEM-REPORT-FORM
%doc TAO/VERSION
%doc TAO/README

%endif

# ---------------- tao-qtresource-devel ----------------

%if %{?_with_qt:1}%{!?_with_qt:0}

%files -n tao-qtresource-devel
%defattr(-,root,root,-)
%{_libdir}/libTAO_QtResource.so
%dir %{_includedir}/tao
%{_includedir}/tao/QtResource/QtResource_Factory.h
%{_includedir}/tao/QtResource/QtResource_Loader.h
%{_includedir}/tao/QtResource/TAO_QtResource_Export.h

%doc TAO/COPYING
%doc TAO/PROBLEM-REPORT-FORM
%doc TAO/VERSION
%doc TAO/README

%endif

# ---------------- tao-tkresource ----------------

%if %{?_with_tk:1}%{!?_with_tk:0}

%files -n tao-tkresource
%defattr(-,root,root,-)
%{_libdir}/libTAO_TkResource.so.%{TAOVERSO}

%doc TAO/COPYING
%doc TAO/PROBLEM-REPORT-FORM
%doc TAO/VERSION
%doc TAO/README

%endif

# ---------------- tao-tkresource-devel ----------------

%if %{?_with_tk:1}%{!?_with_tk:0}

%files -n tao-tkresource-devel
%defattr(-,root,root,-)
%{_libdir}/libTAO_TkResource.so
%dir %{_includedir}/tao
%{_includedir}/tao/TkResource/TkResource_Factory.h
%{_includedir}/tao/TkResource/TkResource_Loader.h
%{_includedir}/tao/TkResource/TAO_TkResource_Export.h

%doc TAO/COPYING
%doc TAO/PROBLEM-REPORT-FORM
%doc TAO/VERSION
%doc TAO/README

%endif

# ---------------- tao-xtresource ----------------

%if %{?_with_xt:1}%{!?_with_xt:0}

%files -n tao-xtresource
%defattr(-,root,root,-)
%{_libdir}/libTAO_XtResource.so.%{TAOVERSO}

%doc TAO/COPYING
%doc TAO/PROBLEM-REPORT-FORM
%doc TAO/VERSION
%doc TAO/README

%endif

# ---------------- tao-xtresource-devel ----------------

%if %{?_with_xt:1}%{!?_with_xt:0}

%files -n tao-xtresource-devel
%defattr(-,root,root,-)
%{_libdir}/libTAO_XtResource.so
%dir %{_includedir}/tao
%{_includedir}/tao/XtResource/XtResource_Factory.h
%{_includedir}/tao/XtResource/XtResource_Loader.h
%{_includedir}/tao/XtResource/TAO_XtResource_Export.h

%doc TAO/COPYING
%doc TAO/PROBLEM-REPORT-FORM
%doc TAO/VERSION
%doc TAO/README

%endif
