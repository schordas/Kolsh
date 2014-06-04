Kolsh
=====

NACHOS project

##kolsh.py
kolsh.py is a python script that makes it easy to code on a local machines and remotely execute code on Aludra.

**Usage**:
`python kolsh.py [--env] [push/pull/watch/init/help] [remote working directory]`

The script has three basic functions:
  - push: &nbsp;&nbsp;&nbsp;pushes the local nachos-csci402 directory to Aludra.
  - pull: &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;pulls the remote nachos-csci402 directory to the current working directory.<br/>
  &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;This ERASES ALL local changes.
  - watch: &nbsp;&nbsp;automatically issues a push command when the local filesystem changes.

Flags:
  - --env: if specified, kolsh will read the remote working directory from the current evnironment.

**Remote Working Directory**<br/>
For kolsh to work properly a remote working directory must be specified as an input parameter or must be defined in the system environment. The remote working directory is the parent directory where your nachos-csci402 directory resides on Aludra. You must provide the full path as given by `pwd` as the remote working directory
<br/><br/>
To setup your environment on your local machine to work with kolsh's --env flag, paste the following at the end of your ~/.bash_profile. Replace remote_working_directory with your remote path.
<br/>
`export KOLSH_REMOTE_WORKING_DIR='remote_working_directory'`

**SSH public/private key authentication**<br/>
For kolsh to work properly you **MUST** set up public/private key pair authentication.<br/>
Steps on how to do so are below.
<br/><br/>
On your local machine open up terminal and ssh into your aludra account and get the full path to your home folder, we will call this **remote_home_folder**. Leave this connection open.

In a seperate terminal window (do not ssh into Aludra), do the following:
```
cd ~/
ls -al
# if the .ssh directory does not exist
mkdir .ssh
cd .ssh
ssh-keygen
# enter aludra for the file to save the key in
# leave the password blank by simply pressing enter
scp aludra.pub [usc login]@aludra.usc.edu:[remote_home_folder]
touch config                  # there is no file extension on config. This is intentional.
open -a "TextEdit" config     # this will open the file config in TextEdit
```

Once TextEdit is open with config, use the following as a template to create your config file. Since we can't see it on github, replace all [tab] with tabs. For kolsh to work properly you **MUST** name your host aludra, just as it's given here.
> Host [tab] aludra <br/>
>	[tab] HostName [tab] aludra.usc.edu <br/>
>	[tab] User [tab] [aludra account name] <br/>
>	[tab] IdentityFile [tab] 	~/.ssh/aludra <br/>


Finally we need to setup your aludra account to use public/private key authentication. Go back to the terminal window you have connected to Aludra). After you login make sure you see your aludra.pub file, if it's not there, go back and get it. Now run the following commands on Aludra:<br/>
```
cd ~/
ls -al
# if the .ssh directory does not exist
mkdir .ssh
mv aludra.pub .ssh/authorized_keys
chmod 700 .ssh
chmod 600 .ssh/authorized_keys
```

Thats it. Now everytime you want to login to Aludra, from your local machine simply type:<br/>
`ssh aludra` 
