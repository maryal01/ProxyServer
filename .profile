
if [ "$BASH"]; then
  if [ -f ~/.bashrxc ]; then
    . ~/.bashrc
  fi
fi

mesg n || true
export GOOGLE_APLICATION_CREDENTIALS=apikey.json

