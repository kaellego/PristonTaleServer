use hmac::{Hmac, Mac};
use sha2::Sha256;

type HmacSha256 = Hmac<Sha256>;

pub const NONCE_SIZE: usize = 32;
pub const MAC_SIZE: usize = 32;

pub fn prove(secret: &[u8], nonce: &[u8; NONCE_SIZE]) -> Option<[u8; MAC_SIZE]> {
    if secret.len() < 32 {
        return None;
    }
    let mut mac = HmacSha256::new_from_slice(secret).ok()?;
    mac.update(nonce);
    let result = mac.finalize().into_bytes();
    Some(result.into())
}

pub fn verify(secret: &[u8], nonce: &[u8; NONCE_SIZE], proof: &[u8]) -> bool {
    let Some(expected) = prove(secret, nonce) else {
        return false;
    };
    let Ok(mut mac) = HmacSha256::new_from_slice(secret) else {
        return false;
    };
    mac.update(nonce);
    mac.verify_slice(proof).is_ok() && proof == expected
}

#[cfg(test)]
mod tests {
    use super::{prove, verify, NONCE_SIZE};

    #[test]
    fn accepts_only_proof_for_same_secret_and_nonce() {
        let secret = b"01234567890123456789012345678901";
        let nonce = [7u8; NONCE_SIZE];
        let proof = prove(secret, &nonce).unwrap();
        assert!(verify(secret, &nonce, &proof));
        assert!(!verify(b"different-secret-012345678901234", &nonce, &proof));
    }
}
