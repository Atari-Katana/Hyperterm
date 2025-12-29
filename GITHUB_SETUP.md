# How to Upload Hyperterm to GitHub

## Prerequisites
- A GitHub account (create one at https://github.com if you don't have one)
- Git installed on your system

## Step-by-Step Instructions

### 1. Create a New Repository on GitHub

1. Go to https://github.com and sign in
2. Click the "+" icon in the top right, select "New repository"
3. Name it (e.g., "hyperterm" or "Hyperterm")
4. Choose public or private
5. **DO NOT** initialize with README, .gitignore, or license (we'll add these locally)
6. Click "Create repository"

### 2. Initialize Git in Your Project (if not already done)

```bash
cd /home/davidjackson/hyperterm
git init
```

### 3. Add All Files

```bash
git add .
```

### 4. Make Your First Commit

```bash
git commit -m "Initial commit: Hyperterm terminal emulator"
```

### 5. Add GitHub Remote

Replace `YOUR_USERNAME` with your GitHub username:

```bash
git remote add origin https://github.com/YOUR_USERNAME/hyperterm.git
```

Or if you prefer SSH (requires SSH key setup):

```bash
git remote add origin git@github.com:YOUR_USERNAME/hyperterm.git
```

### 6. Push to GitHub

```bash
git branch -M main
git push -u origin main
```

You'll be prompted for your GitHub username and password/token.

**Note:** GitHub no longer accepts passwords for HTTPS. You'll need a Personal Access Token:
- Go to GitHub Settings → Developer settings → Personal access tokens → Tokens (classic)
- Generate a new token with `repo` permissions
- Use the token as your password when pushing

## Alternative: Using GitHub CLI (if installed)

If you have `gh` installed:

```bash
gh repo create hyperterm --public --source=. --remote=origin --push
```

## Troubleshooting

- **"Repository not found"**: Check that the repository name matches exactly
- **Authentication failed**: Use a Personal Access Token instead of password
- **"Everything up-to-date"**: Make sure you've committed your changes first

## After Uploading

Your code will be available at:
`https://github.com/YOUR_USERNAME/hyperterm`

You can add a README, license, and other documentation files as needed!
