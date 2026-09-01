# Omarchy — native Arch Linux devcontainer for GitHub Codespaces.
#
# Newest stable: archlinux:latest + the Omarchy stable repo
# (https://pkgs.omarchy.org/stable). For the pre-release channel, change
# both "stable" occurrences below to "edge".
FROM archlinux:latest

# --- Base system: locale, Arch keyring, tooling ---------------------------
RUN sed -i 's/^#en_US.UTF-8/en_US.UTF-8/' /etc/locale.gen \
 && locale-gen \
 && pacman-key --init \
 && pacman-key --populate archlinux \
 && pacman -Syu --noconfirm --needed curl zstd base-devel git sudo

# --- Add the Omarchy repository (stable channel) --------------------------
RUN printf '\n[omarchy]\nServer = https://pkgs.omarchy.org/stable/$arch\n' >> /etc/pacman.conf \
 && pacman -Sy --noconfirm

# --- Bootstrap the Omarchy signing key ------------------------------------
# The omarchy-keyring package is itself signed with the Omarchy key, so we
# resolve its filename from the repo DB, extract the keyring files, and trust
# the key locally. This mirrors what Omarchy's own installer does.
RUN KEYRING_FILE=$(curl -sSL https://pkgs.omarchy.org/stable/x86_64/omarchy.db.tar.zst \
      | zstd -dc | tar --wildcards -xOf - 'omarchy-keyring-*/desc' \
      | grep -A1 '^%FILENAME%' | tail -1) \
 && curl -sSLo /tmp/omarchy-keyring.pkg.tar.zst "https://pkgs.omarchy.org/stable/x86_64/${KEYRING_FILE}" \
 && tar --zstd -xf /tmp/omarchy-keyring.pkg.tar.zst -C /tmp \
 && install -m0644 /tmp/usr/share/pacman/keyrings/omarchy.gpg /usr/share/pacman/keyrings/ \
 && install -m0644 /tmp/usr/share/pacman/keyrings/omarchy-trusted /usr/share/pacman/keyrings/ \
 && install -m0644 /tmp/usr/share/pacman/keyrings/omarchy-revoked /usr/share/pacman/keyrings/ \
 && pacman-key --populate omarchy \
 && rm -rf /tmp/omarchy*

# --- Install Omarchy core + the full base package list ---------------------
# omarchy-keyring is a dependency of omarchy, so pacman re-installs it
# properly (the manually placed keyring files are removed first to avoid
# file conflicts).
COPY omarchy-base.packages /tmp/omarchy-base.packages
RUN rm -f /usr/share/pacman/keyrings/omarchy.gpg /usr/share/pacman/keyrings/omarchy-trusted /usr/share/pacman/keyrings/omarchy-revoked \
 && pacman -S --noconfirm --needed omarchy \
 && pacman -S --noconfirm --needed $(grep -vE '^#|^$' /tmp/omarchy-base.packages | tr '\n' ' ') \
 && pacman -Scc --noconfirm \
 && rm -f /tmp/omarchy-base.packages

# --- Headless desktop mode (Hyprland + wayvnc + noVNC in the browser) -----
# Optional: run `omarchy-desktop` and open port 6080/vnc.html to view a
# software-rendered Hyprland session. Laggy by nature (no GPU) — a demo, not
# a daily driver.
RUN pacman -S --noconfirm --needed wayvnc xorg-xwayland python-pip \
 && python -m venv /opt/venv \
 && /opt/venv/bin/pip install --no-cache-dir websockify \
 && curl -sSL https://github.com/novnc/noVNC/archive/refs/tags/v1.5.0.tar.gz | tar -xz -C /opt \
 && mv /opt/noVNC-1.5.0 /opt/noVNC

COPY desktop/omarchy-desktop /usr/local/bin/omarchy-desktop
RUN chmod +x /usr/local/bin/omarchy-desktop

# --- User + environment ----------------------------------------------------
ENV LANG=en_US.UTF-8
ARG USERNAME=codespace
ARG USER_UID=1000
ARG USER_GID=1000
RUN groupadd -g ${USER_GID} ${USERNAME} \
 && useradd -m -u ${USER_UID} -g ${USER_GID} -s /bin/bash ${USERNAME} \
 && echo "${USERNAME} ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/90-${USERNAME} \
 && chmod 0440 /etc/sudoers.d/90-${USERNAME}

USER ${USERNAME}
WORKDIR /workspaces
