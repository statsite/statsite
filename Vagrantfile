VAGRANTFILE_API_VERSION = "2"

$script = <<CODE

echo
echo Provisioning started...
echo

sudo apt-get update
sudo apt-get -y install build-essential scons python-setuptools lsof
sudo easy_install pip
sudo pip install pytest

cd /vagrant/deps/check-0.9.8/
./configure
make
make install
ldconfig

CODE


Vagrant.configure(VAGRANTFILE_API_VERSION) do |config|
  config.vm.box = "precise64"
  config.vm.box_url = "http://files.vagrantup.com/precise64.box"
  config.vm.network "private_network", ip: "33.33.33.40"

  # Provision using the shell to install fog
  config.vm.provision :shell, :inline => $script
end
