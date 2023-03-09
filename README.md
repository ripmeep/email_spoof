# email_spoof
A simple SMTP email spoofer PoC against weak email servers.

# Setup
First you'll need to install sendmail and the curl dev library (or any other localhost-based SMTP server).

```
apt-get update && apt-get install sendmail libcurl4-openssl-dev -y
```

Make sure it's running

```
sudo service sendmail start && sudo service sendmail status
```

# Get and compile the source

```
git clone https://github.com/ripmeep/email_spoof && cd email_spoof
gcc email_spoof.c -o email_spoof -lcurl
```

All done!

```
./email_spoof [recipient address]
```

Please don't use the irresposibly, I'm not accountable for any wrong-doings by the script kiddie creatures.
