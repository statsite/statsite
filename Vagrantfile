VAGRANTFILE_API_VERSION = "2"

$script = <<CODE

echo
echo Provisioning started...
echo

sudo apt-get update
sudo apt-get -y install build-essential scons python-setuptools lsof git automake
sudo easy_install pip
sudo pip install pytest

cd /vagrant/deps/check-0.9.8/
./configure
make
make install
ldconfig

CODE


Vagrant.configure(VAGRANTFILE_API_VERSION) do |config|
  config.vm.box = "debian/jessie64"
  config.vm.network "private_network", ip: "33.33.33.40"
  config.vm.synced_folder ".", "/vagrant", type: "nfs"

  # Provision using the shell to install fog
  config.vm.provision :shell, :inline => $script


  config.vm.post_up_message = <<MSG

     The box is ready. Statsite is in /vagrant.
     When making changes to configure.ac or Makefile.am, run bootstrap.sh,
     and when you have your dependencies in order, ./configure again.

     Dependencies were already installed, they are:
     - build-essential
     - automake
     - check
     - scons
     - pytest

     To build: use make, to test: make test.


MSG

end
