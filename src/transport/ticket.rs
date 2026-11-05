use std::{
    collections::HashMap,
    sync::{Arc, Mutex},
    time::{Duration, Instant},
};

#[derive(Clone, Default)]
pub struct TicketStore(Arc<Mutex<HashMap<i32, (String, String, Instant)>>>);

impl TicketStore {
    pub fn issue(&self, account: &str, address: &str) -> i32 {
        let mut state = self.0.lock().expect("ticket store poisoned");
        let mut ticket = 1;
        while state.contains_key(&ticket) {
            ticket = ticket.wrapping_add(1).max(1);
        }
        state.insert(
            ticket,
            (
                account.to_owned(),
                address.to_owned(),
                Instant::now() + Duration::from_secs(60),
            ),
        );
        ticket
    }
    pub fn validate(&self, ticket: i32, account: &str, address: &str) -> bool {
        let mut state = self.0.lock().expect("ticket store poisoned");
        let Some((stored_account, stored_address, expires)) = state.get(&ticket) else {
            return false;
        };
        if *expires <= Instant::now() || stored_account != account || stored_address != address {
            return false;
        }
        state.remove(&ticket);
        true
    }
}

#[cfg(test)]
mod tests {
    use super::TicketStore;
    #[test]
    fn ticket_is_bound_and_consumed() {
        let store = TicketStore::default();
        let ticket = store.issue("test", "127.0.0.1");
        assert!(!store.validate(ticket, "other", "127.0.0.1"));
        assert!(store.validate(ticket, "test", "127.0.0.1"));
        assert!(!store.validate(ticket, "test", "127.0.0.1"));
    }
}
